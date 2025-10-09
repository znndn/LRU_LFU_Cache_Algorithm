cmake_minimum_required(VERSION 3.16)
project(LRU_Test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(LRU_Test TestAlgorithm.cpp)

set_target_properties(LRU_Test PROPERTIES CLEAN_DIRECT_OUTPUT 1)
