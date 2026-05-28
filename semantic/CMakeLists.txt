cmake_minimum_required(VERSION 3.10)
project(semantic)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../parser ${CMAKE_BINARY_DIR}/parser)

add_library(semantic_lib
    src/semantic.cpp
    src/builtins.cpp
    src/passes.cpp
    src/type_checker.cpp
)

target_include_directories(semantic_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../parser/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lexer/include
)

target_link_libraries(semantic_lib PUBLIC parser_lib)

add_executable(semantic main.cpp)
target_link_libraries(semantic semantic_lib)
