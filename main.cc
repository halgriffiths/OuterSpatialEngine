//
// Created by henry on 06/12/2021.
//
#include <iostream>
#include "BazaarBot.h"
#include "metrics/metrics.h"
#include <vector>
#include <chrono>
#include <thread>

std::shared_ptr<AITrader> MakeAgent(const std::string& class_name, int curr_id,
                                    std::shared_ptr<AuctionHouse>& auction_house,
                                    std::map<std::string, std::vector<InventoryItem>>& inv,
                                    std::mt19937& gen, int tick_time_ms, Log::LogLevel LOGLEVEL) {
    double STARTING_MONEY = 500.0;
    double MIN_COST = 10;
    std::uniform_real_distribution<> random_money(0.5*STARTING_MONEY, 1.5*STARTING_MONEY); // define the range
    std::uniform_real_distribution<> random_cost(0.9*MIN_COST, 1.1*MIN_COST); // define the range
    if (class_name == "farmer") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleFarmer>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else if (class_name == "woodcutter") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleWoodcutter>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else if (class_name == "miner") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleMiner>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else if (class_name == "refiner") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleRefiner>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else if (class_name == "blacksmith") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleBlacksmith>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else if (class_name == "composter") {
        return CreateAndRegister(curr_id, auction_house, std::make_shared<RoleComposter>(random_cost(gen)), class_name, random_money(gen), 20, inv[class_name], tick_time_ms, LOGLEVEL);
    } else {
        std::cout << "Error: Invalid class type passed to make_agent lambda" << std::endl;
    }
    return std::shared_ptr<AITrader>();
}

std::string ChooseNewClassRandom(std::vector<std::string>& tracked_roles, std::mt19937& gen) {
    std::uniform_int_distribution<> random_job(0, (int) tracked_roles.size() - 1); // define the range
    int new_job = random_job(gen);
    return tracked_roles[new_job];
}

int RandomChoice(int num_weights, std::vector<double>& weights, std::mt19937& gen) {
    double sum_of_weight = 0;
    for(int i=0; i<num_weights; i++) {
        sum_of_weight += weights[i];
    }
    std::uniform_real_distribution<> random(0, sum_of_weight);
    double rnd = random(gen);
    for(int i=0; i<num_weights; i++) {
        if(rnd < weights[i])
            return i;
        rnd -= weights[i];
    }
    return -1;
}

std::string GetProducer(std::string& commodity) {
    if (commodity == "food") {
        return "farmer";
    } else if (commodity == "fertilizer") {
        return "composter";
    } else if (commodity == "wood") {
        return "woodcutter";
    } else if (commodity == "ore") {
        return "miner";
    } else if (commodity == "metal") {
        return "refiner";
    } else if (commodity == "tools") {
        return "blacksmith";
    } else {
        return "null";
    }
}
std::string ChooseNewClassWeighted(std::vector<std::string>& tracked_goods, std::shared_ptr<AuctionHouse>& auction_house, std::mt19937& gen) {
    std::vector<double> weights;
    double gamma = -0.02;
    //auction house ticks at 10ms
    int lookback_time_ms = 1000;
    for (auto& commodity : tracked_goods) {
        double supply = auction_house->t_AverageHistoricalSupply(commodity, lookback_time_ms);
//        double supply = auction_house->AverageHistoricalAsks(commodity, 100) - auction_house->AverageHistoricalBids(commodity, 100);
        weights.push_back(std::exp(gamma*supply));
    }
    int choice = RandomChoice((int) weights.size(),  weights, gen);
    assert(choice != -1);
    return GetProducer(tracked_goods[choice]);
}

void Run(double duration_s, double animation_fps, double trader_tps) {
    int NUM_TRADERS_EACH_TYPE = 10;
    int TARGET_NUM_TRADERS = 120;
    int DURATION_MS = (int) duration_s*1000; //60 second simulation

    int TRADER_TICK_TIME_MS = 1000/trader_tps;

    int TARGET_STEPTIME_MS = 10;
    int TARGET_ANIMATION_MS;
    if (animation_fps > 0) {
        TARGET_ANIMATION_MS = 1000/animation_fps;
    } else {
        TARGET_ANIMATION_MS = 1000;
    }
    int writes_per_animation = 10;

    auto trader_log_level = Log::DEBUG;
    auto AH_log_level = Log::DEBUG;

    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    std::random_device rd; // obtain a random number from hardware
    std::mt19937 gen(rd()); // seed the generator

    std::vector<std::string> tracked_goods = {"food", "wood", "fertilizer", "ore", "metal", "tools"};
    std::vector<std::string> tracked_roles = {"farmer", "woodcutter", "composter", "miner", "refiner", "blacksmith"};

    auto file_mutex = std::make_shared<std::mutex>();
    auto metrics_start_time = to_unix_timestamp_ms(std::chrono::high_resolution_clock::now());
    auto global_metrics = GlobalMetrics(metrics_start_time, tracked_goods, tracked_roles, file_mutex);

    // --- SET UP DEFAULT COMMODITIES ---
    std::map<std::string, Commodity> comm;
    {
        comm.emplace("food", Commodity("food", 0.5));
        comm.emplace("wood", Commodity("wood", 1));
        comm.emplace("ore", Commodity("ore", 1));
        comm.emplace("metal", Commodity("metal", 1));
        comm.emplace("tools", Commodity("tools", 1));
        comm.emplace("fertilizer", Commodity("fertilizer", 0.1));
    }
    // --- SET UP DEFAULT INVENTORIES ---
    std::map<std::string, std::vector<InventoryItem>> inv;
    std::vector<InventoryItem> player_inv = {{comm["food"], 10, 10},
                                             {comm["tools"], 10, 10},
                                             {comm["wood"], 10, 10},
                                             {comm["fertilizer"], 10, 10}};
    {
        inv.emplace("farmer", std::vector<InventoryItem>{{comm["food"], 0, 0},
                                                         {comm["tools"], 1, 2},
                                                         {comm["wood"], 1, 6},
                                                         {comm["fertilizer"], 1, 6}});

        inv.emplace("miner", std::vector<InventoryItem>{{comm["food"], 1, 6},
                                                        {comm["tools"], 1, 2},
                                                        {comm["ore"], 0, 0}});

        inv.emplace("refiner", std::vector<InventoryItem>{{comm["food"], 1, 6},
                                                          {comm["tools"], 1, 2},
                                                          {comm["ore"], 1, 10},
                                                          {comm["metal"], 0, 0}});

        inv.emplace("woodcutter", std::vector<InventoryItem>{{comm["food"], 1, 6},
                                                             {comm["tools"], 1, 2},
                                                             {comm["wood"], 0, 0}});

        inv.emplace("blacksmith", std::vector<InventoryItem>{{comm["food"], 1, 6},
                                                             {comm["tools"], 0, 0},
                                                             {comm["metal"], 0, 10}});

        inv.emplace("composter", std::vector<InventoryItem>{{comm["food"], 1, 6},
                                                            {comm["fertilizer"], 0, 0}});
    }

    // --- SET UP AUCTION HOUSE ---
    int max_id = 0;
    auto auction_house = std::make_shared<AuctionHouse>(max_id, AH_log_level);
    max_id++;
    for (auto& item : comm) {
        auction_house->RegisterCommodity(item.second);
    }
    std::thread auction_house_thread(&AuctionHouse::Tick, auction_house, DURATION_MS);
    // --- SET UP AI TRADERS ---
    for (int i = 0; i < NUM_TRADERS_EACH_TYPE; i++) {
        for (auto& role : tracked_roles) {
            auto new_agent = MakeAgent(role, max_id, auction_house, inv, gen, TRADER_TICK_TIME_MS, trader_log_level);
            max_id++;
            std::thread new_agent_thread(&AITrader::Tick, new_agent);
            new_agent_thread.detach();
        }
    }
    for (int i = 0; i < 20; i++) {
        auto new_composter = MakeAgent("composter", max_id, auction_house, inv, gen, TRADER_TICK_TIME_MS, trader_log_level);
        max_id++;
        std::thread new_composter_thread(&AITrader::Tick, new_composter);
        new_composter_thread.detach();
    }
//    // --- SET UP FAKE TRADER ---
//    auto fake_trader = std::make_shared<FakeTrader>(max_id, auction_house);
//    {
//        fake_trader->SendMessage(*Message(max_id).AddRegisterRequest(std::move(RegisterRequest(max_id, fake_trader))), auction_house->id);
//        fake_trader->Tick();
//        //fake_trader->RegisterShortage("fertilizer", 3, 620, 50);
//        //fake_trader->RegisterSurplus("fertilizer", -0.9, 220, 50);
//        max_id++;
//    }

    auto player_trader = std::make_shared<PlayerTrader>(metrics_start_time, max_id, auction_house, 100, 50, player_inv, tracked_goods, tracked_roles, Log::DEBUG);
    {
        player_trader->SendMessage(*Message(max_id).AddRegisterRequest(std::move(RegisterRequest(max_id, player_trader))), auction_house->id);
        max_id++;
    }

    global_metrics.CollectMetrics(auction_house);
    auto global_display = GlobalDisplay(metrics_start_time, TARGET_ANIMATION_MS, file_mutex, tracked_goods);
    if (animation_fps <= 0) {
        global_display.active = false;
    }
    // --- MAIN LOOP ---
    std::cout << std::fixed;
    std::cout << std::setprecision(2);
    int elapsed = 0;
    int curr_tick = 0;
    int prev_write_time = 0;
    int write_step = (int) TARGET_ANIMATION_MS / writes_per_animation ;
    while (elapsed < DURATION_MS) {
        auto t1 = std::chrono::high_resolution_clock::now();
        curr_tick++;
        int num_traders = auction_house->GetNumTraders();
        if (num_traders < TARGET_NUM_TRADERS) {
            for (int i = 0; i < TARGET_NUM_TRADERS- num_traders; i++) {
                auto new_role = ChooseNewClassWeighted(tracked_goods, auction_house, gen);
                auto new_agent = MakeAgent(new_role, max_id, auction_house, inv, gen, TRADER_TICK_TIME_MS, trader_log_level);
                max_id++;
                std::thread new_agent_thread(&AITrader::Tick, new_agent);
                new_agent_thread.detach();
            }
        }
        if (elapsed > prev_write_time + write_step) {
            global_metrics.update_datafiles(prev_write_time);
            prev_write_time = elapsed;
        }
        global_metrics.CollectMetrics(auction_house);
        std::chrono::duration<double, std::milli> ms_double = std::chrono::high_resolution_clock::now() - t1;
        int working_frametime_ms = (int) ms_double.count();

        if (working_frametime_ms < TARGET_STEPTIME_MS) {
            std::this_thread::sleep_for(std::chrono::milliseconds{TARGET_STEPTIME_MS - working_frametime_ms});
        } else {
            //std::cout << "[DRIVER] Overrun frametime for tick " << curr_tick << ": " << working_frametime_ms << "/" << TARGET_STEPTIME_MS << std::endl;
        }
        ms_double = std::chrono::high_resolution_clock::now() - t1;
        int frametime_ms = (int) ms_double.count();
        elapsed += frametime_ms;
//        {
//            if (animation && curr_tick > WINDOW_SIZE) {
//                global_metrics.update_datafiles();
//                display_plot(global_metrics, WINDOW_SIZE);
//            }
//        }
    }
    std::cout << "Manually shutdown AH" << std::endl;
    auction_house->Shutdown();
    auction_house_thread.join();
    global_display.Shutdown();

    for (auto& good : tracked_goods) {
        std::cout << "\t\t\t" << good;
    }
    std::cout << std::endl;
    for (auto& good : tracked_goods) {
        double price = auction_house->AverageHistoricalPrice(good, 10);

        std::cout << "\t\t$" << price;
        double pc_change = auction_house->history.prices.t_percentage_change(good, 10000);
        if (pc_change < 0) {
            //▼
            std::cout << "\033[1;31m(▼" << pc_change << "%)\033[0m";
        } else if (pc_change > 0) {
            //▲
            std::cout << "\033[1;32m(▲" << pc_change << "%)\033[0m";
        } else {
            std::cout << "(" << pc_change << "%)";
        }
    }

    std::cout << "\nAverage age on death: " << global_metrics.avg_lifespan << std::endl;
//    for (auto& role : tracked_roles) {
//        std::cout << role << ": " << global_metrics.age_per_class[role] << "(" <<global_metrics.deaths_per_class[role] <<" total)" << std::endl;
//    }

    std::cout << "Total auction house profit :" << auction_house->spread_profit << std::endl;
    auction_house.reset();
    std::cout << "Finished" << std::endl;
}

// ---------------- MAIN ----------
int main(int argc, char *argv[]) {
    double duration_s = (argc > 1) ? std::stod(std::string(argv[1])) : 60;
    double animation_fps = (argc > 2) ? std::stod(std::string(argv[2])) : 0;
    double trader_tps = (argc > 3) ? std::stod(std::string(argv[3])) : 2;
    Run(duration_s, animation_fps, trader_tps);
    return 0;
}