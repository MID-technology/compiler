cmake_minimum_required(VERSION 3.13)
project(codegen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM (prefer Homebrew's keg-only llvm on macOS)
if(NOT DEFINED LLVM_DIR)
    foreach(_p /opt/homebrew/opt/llvm /usr/local/opt/llvm)
        if(EXISTS "${_p}/lib/cmake/llvm")
            set(LLVM_DIR "${_p}/lib/cmake/llvm")
            break()
        endif()
    endforeach()
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} (${LLVM_DIR})")

add_subdirectory(../semantic ${CMAKE_BINARY_DIR}/semantic)

add_library(codegen_lib
    src/codegen.cpp
)

target_include_directories(codegen_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../semantic/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../parser/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lexer/include
    ${LLVM_INCLUDE_DIRS}
)

target_compile_definitions(codegen_lib PUBLIC ${LLVM_DEFINITIONS})
target_link_libraries(codegen_lib PUBLIC semantic_lib)

llvm_map_components_to_libnames(LLVM_LIBS core support irreader)
target_link_libraries(codegen_lib PUBLIC ${LLVM_LIBS})

add_executable(olc main.cpp)
target_link_libraries(olc codegen_lib)

# Runtime library (compiled by clang to object)
find_program(CLANG_EXE NAMES clang HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
if(NOT CLANG_EXE)
    set(CLANG_EXE clang)
endif()

set(RUNTIME_SRC ${CMAKE_CURRENT_SOURCE_DIR}/runtime/runtime.c)
set(RUNTIME_OBJ ${CMAKE_CURRENT_BINARY_DIR}/runtime.o)

add_custom_command(
    OUTPUT ${RUNTIME_OBJ}
    COMMAND ${CLANG_EXE} -c -O2 ${RUNTIME_SRC} -o ${RUNTIME_OBJ}
    DEPENDS ${RUNTIME_SRC}
)
add_custom_target(ol_runtime ALL DEPENDS ${RUNTIME_OBJ})
