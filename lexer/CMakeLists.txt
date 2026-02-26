cmake_minimum_required(VERSION 3.15)
project(OLexer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_library(lexer_lib
    src/token.cpp
    src/lexer.cpp
)

target_include_directories(lexer_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(lexer_demo
    src/main.cpp
)

target_link_libraries(lexer_demo PRIVATE lexer_lib)

# хз почему красным горит, все работает
enable_testing()
add_subdirectory(tests)


file(GLOB EXAMPLE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tests/*.ol")

add_custom_target(test_examples
    COMMAND ${CMAKE_COMMAND} -E echo "Testing example files..."
    DEPENDS lexer_demo
)

foreach(example_file ${EXAMPLE_FILES})
    get_filename_component(example_name ${example_file} NAME)
    add_custom_command(TARGET test_examples POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Testing ${example_name}..."
        COMMAND $<TARGET_FILE:lexer_demo> ${example_file} > /dev/null || ${CMAKE_COMMAND} -E echo "Failed: ${example_name}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

# Запускайте это чтобы чекнуть все Данины тесты 
add_custom_target(test_all
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running unit tests ==="
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "=== Testing example files ==="
    DEPENDS lexer_tests test_examples
)