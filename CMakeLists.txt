cmake_minimum_required(VERSION 3.14)
project(BPlusTree VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Header-only library (template implementation)
add_library(bplustree INTERFACE)
target_include_directories(bplustree INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Example executables
add_executable(demo examples/demo.cpp)
target_link_libraries(demo bplustree)

add_executable(iterator_demo examples/iterator_demo.cpp)
target_link_libraries(iterator_demo bplustree)

# Enable testing
enable_testing()

# Test executables
add_executable(test_insert tests/test_insert.cpp)
target_link_libraries(test_insert bplustree)
add_test(NAME test_insert COMMAND test_insert)

add_executable(test_delete tests/test_delete.cpp)
target_link_libraries(test_delete bplustree)
add_test(NAME test_delete COMMAND test_delete)

add_executable(test_search tests/test_search.cpp)
target_link_libraries(test_search bplustree)
add_test(NAME test_search COMMAND test_search)

add_executable(test_range tests/test_range.cpp)
target_link_libraries(test_range bplustree)
add_test(NAME test_range COMMAND test_range)

add_executable(test_min_order tests/test_min_order.cpp)
target_link_libraries(test_min_order bplustree)
add_test(NAME test_min_order COMMAND test_min_order)

add_executable(test_edge_cases tests/test_edge_cases.cpp)
target_link_libraries(test_edge_cases bplustree)
add_test(NAME test_edge_cases COMMAND test_edge_cases)

add_executable(test_stress tests/test_stress.cpp)
target_link_libraries(test_stress bplustree)
add_test(NAME test_stress COMMAND test_stress)

add_executable(test_validate tests/test_validate.cpp)
target_link_libraries(test_validate bplustree)
add_test(NAME test_validate COMMAND test_validate)

add_executable(test_iterator tests/test_iterator.cpp)
target_link_libraries(test_iterator bplustree)
add_test(NAME test_iterator COMMAND test_iterator)
