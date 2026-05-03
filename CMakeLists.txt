cmake_minimum_required(VERSION 3.16)
project(mcpp98 VERSION 0.1.0 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 98)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(mcpp98_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /Zc:__cplusplus)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -pedantic -Werror)
    endif()
endfunction()

add_library(mcpp98
    src/error.cpp
    src/json.cpp
    src/rpc.cpp
)
target_include_directories(mcpp98 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
mcpp98_warnings(mcpp98)

include(CTest)
if(BUILD_TESTING)
    add_executable(mcpp98_tests
        tests/main.cpp
        tests/test_json.cpp
    )
    target_link_libraries(mcpp98_tests PRIVATE mcpp98)
    target_include_directories(mcpp98_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    mcpp98_warnings(mcpp98_tests)
    add_test(NAME mcpp98_tests COMMAND mcpp98_tests)
endif()

