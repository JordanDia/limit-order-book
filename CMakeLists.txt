cmake_minimum_required(VERSION 3.20)
project (orderbook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(orderbook src/OrderBook.cpp)
target_include_directories(orderbook PUBLIC include)
target_compile_options(orderbook PRIVATE -Wall -Wextra)

include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(orderbook_tests tests/OrderBookTests.cpp)
target_link_libraries(orderbook_tests PRIVATE orderbook GTest::gtest_main)
target_compile_options(orderbook_tests PRIVATE -Wall -Wextra)
include(GoogleTest)
gtest_discover_tests(orderbook_tests)