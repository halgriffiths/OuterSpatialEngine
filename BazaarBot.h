#ifndef BAZAAR_BOT_H
#define BAZAAR_BOT_H


#include "common/concurrency.h"

#include "common/commodity.h"
#include "traders/inventory.h"

#include "common/agent.h"

#include "auction/auction_house.h"
#include "common/messages.h"
#include "metrics/logger.h"
#include "traders/AI_trader.h"
#include "traders/fake_trader.h"
#include "traders/human_trader.h"
#include "traders/roles.h"

#include "metrics/display.h"

std::shared_ptr<AITrader> CreateAndRegisterBasic(int id,
                                                    const std::vector<std::pair<Commodity, int>>& inv,
                                                    const std::shared_ptr<AuctionHouse>& auction_house) {

    std::vector<InventoryItem> inv_vector;
    for (const auto &item : inv) {
        inv_vector.emplace_back(item.first.name, item.second);
    }
    auto trader = std::make_shared<AITrader>(id, auction_house, std::nullopt, "test_class", 100.0, 50, inv_vector, Log::WARN);

    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->TickOnce();
    return trader;
}
std::shared_ptr<AITrader> CreateAndRegister(int id,
                                               const std::shared_ptr<AuctionHouse>& auction_house,
                                               std::shared_ptr<Role> AI_logic,
                                               const std::string& name,
                                               double starting_money,
                                               double inv_capacity,
                                               const std::vector<InventoryItem> inv,
                                               int tick_time_ms,
                                               Log::LogLevel log_level
) {

    auto trader = std::make_shared<AITrader>(id, auction_house, std::move(AI_logic), name, starting_money, inv_capacity, inv, tick_time_ms,  log_level);
    trader->SendMessage(*Message(id).AddRegisterRequest(std::move(RegisterRequest(trader->id, trader))), auction_house->id);
    trader->TickOnce();
    return trader;
}



#endif //BAZAAR_BOT_H
