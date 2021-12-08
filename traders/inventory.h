//
// Created by henry on 06/12/2021.
//
#ifndef CPPBAZAARBOT_INVENTORY_H
#define CPPBAZAARBOT_INVENTORY_H
#include "../common/commodity.h"
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <unordered_set>

class InventoryItem {
public:
    InventoryItem() = default;
    InventoryItem(std::string commodity_name) : name(std::move(commodity_name)) {};

    InventoryItem(std::string commodity_name, int starting_quantity)
        : name(std::move(commodity_name))
        , stored(starting_quantity) {};

    std::string name = "default_commodity";
    int stored = 0;
    int ideal_quantity= 0;
    double original_cost = 0;
    double size = 1;            //default to 1 unit of commodity per unit of inventory space
};

class Inventory {
public:
    double max_size = 50;
    std::map<std::string, InventoryItem> inventory;

    Inventory() = default;
    Inventory(double max_size, const std::vector<InventoryItem> &starting_inv)
    : max_size(max_size) {
        for (const auto& item : starting_inv) {
            inventory[item.name] = item;
        }
    };

    void SetItem(const std::string& name, InventoryItem &new_item) {
        inventory[name] = new_item;
    }

    bool SetIdeal(const std::string& name, int ideal_quantity) {
        if (inventory.count(name) != 1) {
            return false;// no entry found
        }
        inventory[name].ideal_quantity = ideal_quantity;
        return true;
    }

    std::optional<int> Query(const std::string& name) {
        if (inventory.count(name) != 1) {
            return std::nullopt;; // no entry found
        }
        return inventory[name].stored;
    }

    std::optional<InventoryItem> GetItem(const std::string& name) {
        if (inventory.count(name) != 1) {
            return std::nullopt;// no entry found
        }
        return inventory[name];
    }

    double GetUsedSpace() {
        double used = 0;
        for (const auto &item : inventory) {
            used += item.second.stored * item.second.size;
        }
        return used;
    }

    double GetEmptySpace() {
        return max_size - GetUsedSpace();
    }

    std::optional<double> ChangeItem(const std::string& name, int delta, double unit_cost) {
        if (inventory.count(name) != 1) {
            return std::nullopt;// no entry found
        }
        InventoryItem *item_entry = &inventory[name];
        if (unit_cost > 0) {
            if (item_entry->stored <= 0) {
                item_entry->stored = delta;
                item_entry->original_cost = unit_cost;
            } else {
                // average out original cost
                item_entry->original_cost = (item_entry->stored * item_entry->original_cost + delta * unit_cost) / (item_entry->stored + delta);
                item_entry->stored += delta;
            }
        } else {
            item_entry->stored += delta;
            // item_entry->original_cost is unchanged?
        }

        if (item_entry->stored < 0) {
            item_entry->stored = 0;
            item_entry->original_cost = 0;
        }
        return item_entry->original_cost;//return current unit cost
    }

    double surplus(const std::string& name) {
        auto stored = Query(name);
        if (!stored) {
            return 0;
        }

        int target = inventory[name].ideal_quantity;
        if (*stored > target) {
            return *stored - target;
        }
        return 0;
    }

    double shortage(const std::string& name) {
        auto stored = Query(name);
        if (!stored) {
            return 0;
        }

        int target = inventory[name].ideal_quantity;
        if (*stored < target) {
            return target - *stored;
        }
        return 0;
    }
};
#endif//CPPBAZAARBOT_INVENTORY_H
