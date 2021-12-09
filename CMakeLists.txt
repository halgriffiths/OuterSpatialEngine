cmake_minimum_required(VERSION 3.19)
project(CppBazaarBot)

set(CMAKE_CXX_STANDARD 20)

add_library(bazaarbot BazaarBot.h traders/trader.h common/agent.h common/messages.h auction/auction_house.h metrics/metrics.h traders/inventory.h common/commodity.h common/history.h)
set_target_properties(bazaarbot PROPERTIES LINKER_LANGUAGE CXX)

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cc)
# Add executable target with source files listed in SOURCE_FILES variable
add_executable(simple_example ${SOURCE_FILES})