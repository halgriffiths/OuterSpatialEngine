cmake_minimum_required(VERSION 3.19)
project(CppBazaarBot)
set(CMAKE_CXX_STANDARD 20)

############# LIBRARY ############
add_library(bazaarbot BazaarBot.h traders/trader.h common/agent.h common/messages.h auction/auction_house.h metrics/metrics.h traders/inventory.h common/commodity.h common/history.h)
set_target_properties(bazaarbot PROPERTIES LINKER_LANGUAGE CXX)

############# DRIVER ############
# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cc)
add_executable(simple_example ${SOURCE_FILES})

############# TESTS #############
# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})
set(TEST_SOURCES testing.cc)
add_executable(runTests ${TEST_SOURCES})
set_target_properties(runTests PROPERTIES LINKER_LANGUAGE CXX)
# Link runTests with newly generated bazaarbot library
target_link_libraries(runTests ${CMAKE_SOURCE_DIR}/libbazaarbot.a ${GTEST_LIBRARIES} pthread)