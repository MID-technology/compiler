add_executable(lexer_tests
    test_lexer.cpp
)

target_link_libraries(lexer_tests PRIVATE lexer_lib)

add_test(NAME lexer_tests COMMAND lexer_tests)