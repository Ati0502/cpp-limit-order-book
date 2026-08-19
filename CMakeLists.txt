cmake_minimum_required(VERSION 3.14)
project(TradingEngine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configure include directory
include_directories(include)

# Source files
set(SOURCE_FILES
    src/OrderBook.cpp
    src/MatchingEngine.cpp
    src/Exchange.cpp
    src/OrderGenerator.cpp
    src/MarketMaker.cpp
)

# Core Library
add_library(trading_engine_lib STATIC ${SOURCE_FILES})

# Main executable (Interactive CLI)
add_executable(trading_simulator main.cpp)
target_link_libraries(trading_simulator PRIVATE trading_engine_lib)

# Benchmark executable
add_executable(benchmarks benchmarks/Benchmark.cpp)
target_link_libraries(benchmarks PRIVATE trading_engine_lib)

# Unit Tests (using GoogleTest)
option(BUILD_TESTING "Build the testing suite" ON)
if(BUILD_TESTING)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/refs/heads/main.zip
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(unit_tests
        tests/OrderBookTests.cpp
        tests/MatchingEngineTests.cpp
        tests/MarketMakerTests.cpp
    )
    target_link_libraries(unit_tests PRIVATE trading_engine_lib gtest_main gtest)
    include(GoogleTest)
    gtest_discover_tests(unit_tests)
endif()
