#ifndef BAZAAR_BOT_H
#define BAZAAR_BOT_H



#include "common/commodity.h"
#include "traders/inventory.h"

#include "common/agent.h"

#include "traders/trader.h"
#include "auction/auction_house.h"
#include "common/messages.h"
#include "metrics/metrics.h"


std::shared_ptr<BasicTrader> CreateAndRegisterBasic(int id,
                                                    const std::vector<std::pair<Commodity, int>>& inv,
                                                    const std::shared_ptr<AuctionHouse>& auction_house) {

    std::vector<InventoryItem> inv_vector;
    for (const auto &item : inv) {
        inv_vector.emplace_back(item.first.name, item.second);
    }
    auto trader = std::make_shared<BasicTrader>(id, auction_house, std::nullopt, "test_class", 100.0, 50, inv_vector, Log::WARN);

    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->Tick();
    return trader;
}
std::shared_ptr<BasicTrader> CreateAndRegister(int id,
                                               const std::shared_ptr<AuctionHouse>& auction_house,
                                               std::shared_ptr<Role> AI_logic,
                                               const std::string& name,
                                               double starting_money,
                                               double inv_capacity,
                                               const std::vector<InventoryItem> inv,
                                               Log::LogLevel log_level
) {

    auto trader = std::make_shared<BasicTrader>(id, auction_house, std::move(AI_logic), name, starting_money, inv_capacity, inv, log_level);
    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->Tick();
    return trader;
}
std::shared_ptr<BasicTrader> CreateAndRegisterFarmer(int id,
                                                     const std::vector<InventoryItem>& inv,
                                                     const std::shared_ptr<AuctionHouse>& auction_house) {
    std::shared_ptr<Role> AI_logic;
    AI_logic = std::make_shared<RoleFarmer>();
    return CreateAndRegister(id, auction_house, AI_logic, "farmer", 100.0, 50, inv, Log::WARN);
}


#endif //BAZAAR_BOT_H
