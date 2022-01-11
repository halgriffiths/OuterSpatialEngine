cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)


############# LIBRARY ############
project(CppBazaarBot)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(bazaarbot outerspatial_engine.h traders/AI_trader.h common/agent.h common/messages.h auction/auction_house.h metrics/logger.h traders/inventory.h common/commodity.h common/history.h traders/roles.h traders/fake_trader.h metrics/display.h common/concurrency.h traders/human_trader.h)
set_target_properties(bazaarbot PROPERTIES LINKER_LANGUAGE CXX)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(bazaarbot PRIVATE Threads::Threads)

############# DRIVER ############

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cc metrics/metrics.h)
add_executable(driver ${SOURCE_FILES})
target_compile_features(driver PRIVATE cxx_std_17)
target_link_libraries(driver PRIVATE
        Threads::Threads
        )
