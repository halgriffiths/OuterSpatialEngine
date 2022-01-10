cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)


############# LIBRARY ############
project(CppBazaarBot)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(bazaarbot BazaarBot.h traders/AI_trader.h common/agent.h common/messages.h auction/auction_house.h metrics/logger.h traders/inventory.h common/commodity.h common/history.h traders/roles.h traders/fake_trader.h metrics/display.h common/concurrency.h traders/human_trader.h)
set_target_properties(bazaarbot PROPERTIES LINKER_LANGUAGE CXX)


set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(bazaarbot PRIVATE Threads::Threads)

############# DRIVER ############

# GNUPLOT stuff
find_package(Boost REQUIRED COMPONENTS
        iostreams system filesystem
        )

# Target.
add_library(gnuplot_iostream INTERFACE)
target_include_directories(gnuplot_iostream INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
        $<INSTALL_INTERFACE:>)
target_link_libraries(gnuplot_iostream INTERFACE
        Boost::iostreams
        Boost::system
        Boost::filesystem
        )

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.cc metrics/metrics.h)
add_executable(driver ${SOURCE_FILES})
target_compile_features(driver PRIVATE cxx_std_17)
target_link_libraries(driver PRIVATE
        gnuplot_iostream
        util
        Threads::Threads
        )

############## TESTS #############
## Locate GTest
#find_package(GTest REQUIRED)
#include_directories(${GTEST_INCLUDE_DIRS})
#set(TEST_SOURCES testing.cc)
#add_executable(runTests ${TEST_SOURCES})
#set_target_properties(runTests PROPERTIES LINKER_LANGUAGE CXX)
## Link runTests with newly generated bazaarbot library
#add_dependencies(runTests bazaarbot)
#target_link_libraries(runTests ${CMAKE_SOURCE_DIR}/libbazaarbot.a ${GTEST_LIBRARIES} pthread)