cmake_minimum_required(VERSION 3.5)

project(BitArrayFastRotate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(BitArrayFastRotate testrot.cpp BitArray.hpp)
add_executable(BenchmarkRotate benchrot.cpp BitArray.hpp)

target_link_libraries(BenchmarkRotate benchmark)

