//
// Created by henry on 14/12/2021.
//

#ifndef CPPBAZAARBOT_ROLES_H
#define CPPBAZAARBOT_ROLES_H

#include "AI_trader.h"

class EmptyRole : public Role {
    void TickRole(AITrader& trader) override {};
};

class RoleFarmer : public Role {
public:
    void TickRole(AITrader& trader) override {
        bool has_wood = (0 < trader.Query("wood"));
        bool has_tools = (0 < trader.Query("tools"));
        bool has_fertilizer = (0 < trader.Query("fertilizer"));
        bool too_much_food = (3*trader.GetIdeal("food") < trader.Query("food"));
        // Stop producing if you have way too many goods (3x ideal)
        if (!has_fertilizer) {
            LoseMoney(trader, trader.IDLE_TAX);
            return;
        }
        Consume(trader, "fertilizer", 1);
        if (has_tools && has_wood) {
            // 10% chance tools break
            Consume(trader, "tools", 1, 0.1);
            Consume(trader, "wood", 1);
            Produce(trader, "food", 6);
        } else if (has_wood){
            Consume(trader, "wood", 1);
            Produce(trader, "food", 3);
        } else {
            Produce(trader, "food", 1);
        }
    }
};

class RoleWoodcutter : public Role {
public:
    void TickRole(AITrader& trader) override {
        bool has_food = (0 < trader.Query("food"));
        bool has_tools = (0 < trader.Query("tools"));
        bool too_much_wood = (3*trader.GetIdeal("wood") < trader.Query("wood"));
        // Stop producing if you have way too many goods (3x ideal) and some money (5 days worth)
        if (!has_food) {
            LoseMoney(trader, trader.IDLE_TAX);//$2 idleness fine
            return;
        }

        if (has_tools) {
            // 10% chance tools break
            Consume(trader, "tools", 1, 0.1);
            Consume(trader, "food", 1);
            Produce(trader, "wood", 2);
        } else {
            Consume(trader, "food", 1);
            Produce(trader, "wood", 1);
        }
    }
};

class RoleComposter : public Role {
public:
    void TickRole(AITrader& trader) override {
        bool has_food = (0 < trader.Query("food"));
        if (!has_food) {
            LoseMoney(trader, trader.IDLE_TAX);//$2 idleness fine
            return;
        }
        Consume(trader, "food", 1);
        Produce(trader, "fertilizer", 1);
    }
};
#endif//CPPBAZAARBOT_ROLES_H