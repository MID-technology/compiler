cmake_minimum_required(VERSION 3.10)
project(parser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(../lexer ${CMAKE_BINARY_DIR}/lexer)

add_library(parser_lib
    src/parser.cpp
)

target_include_directories(parser_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lexer/include
)

target_link_libraries(parser_lib PUBLIC lexer_lib)

add_executable(parser main.cpp)
target_link_libraries(parser parser_lib)