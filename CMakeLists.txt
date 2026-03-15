cmake_minimum_required(VERSION 3.20)
project(trading_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(engine
  src/main.cpp
  src/orderbook.cpp
  src/engine.cpp
  src/market_data.cpp
)

target_include_directories(engine PRIVATE include)
target_compile_options(engine PRIVATE -O2)
find_package(Threads REQUIRED)
target_link_libraries(engine PRIVATE Threads::Threads)
