cmake_minimum_required(VERSION 3.16)
project(mcpp98 VERSION 0.1.0 LANGUAGES CXX)

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 98)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 98 is numerically larger than 11, so compare a rank, not the raw standard.
if(CMAKE_CXX_STANDARD EQUAL 98 OR CMAKE_CXX_STANDARD EQUAL 03)
    set(MCPP98_STD_RANK 1998)
elseif(CMAKE_CXX_STANDARD EQUAL 11)
    set(MCPP98_STD_RANK 2011)
elseif(CMAKE_CXX_STANDARD EQUAL 14)
    set(MCPP98_STD_RANK 2014)
elseif(CMAKE_CXX_STANDARD EQUAL 17)
    set(MCPP98_STD_RANK 2017)
elseif(CMAKE_CXX_STANDARD EQUAL 20)
    set(MCPP98_STD_RANK 2020)
elseif(CMAKE_CXX_STANDARD EQUAL 23 OR CMAKE_CXX_STANDARD EQUAL 26)
    set(MCPP98_STD_RANK 2023)
else()
    message(FATAL_ERROR "Unsupported CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}")
endif()

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
    src/tool.cpp
    src/session.cpp
    src/server.cpp
    src/stdio_transport.cpp
)
target_include_directories(mcpp98 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
mcpp98_warnings(mcpp98)

add_executable(echo_server examples/echo_server.cpp)
target_link_libraries(echo_server PRIVATE mcpp98)
mcpp98_warnings(echo_server)

if(MCPP98_STD_RANK GREATER_EQUAL 2011)
    add_executable(echo_server_11 examples/echo_server_11.cpp)
    target_link_libraries(echo_server_11 PRIVATE mcpp98)
    mcpp98_warnings(echo_server_11)
endif()

if(MCPP98_STD_RANK GREATER_EQUAL 2023)
    add_executable(echo_server_23 examples/echo_server_23.cpp)
    target_link_libraries(echo_server_23 PRIVATE mcpp98)
    mcpp98_warnings(echo_server_23)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(mcpp98_tests
        tests/main.cpp
        tests/test_json.cpp
        tests/test_session.cpp
        tests/test_server.cpp
    )
    if(MCPP98_STD_RANK GREATER_EQUAL 2011)
        target_sources(mcpp98_tests PRIVATE tests/test_ext_11.cpp tests/test_tool_builder.cpp)
    endif()
    if(MCPP98_STD_RANK GREATER_EQUAL 2023)
        target_sources(mcpp98_tests PRIVATE tests/test_ext_23.cpp)
    endif()
    target_link_libraries(mcpp98_tests PRIVATE mcpp98)
    target_include_directories(mcpp98_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
    mcpp98_warnings(mcpp98_tests)
    add_test(NAME mcpp98_tests COMMAND mcpp98_tests)
    add_test(
        NAME stdio_smoke
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/stdio_smoke.sh $<TARGET_FILE:echo_server>
    )
endif()

include(GNUInstallDirs)

file(RELATIVE_PATH MCPP98_INCLUDE_FROM_LIBDIR
    "/prefix/${CMAKE_INSTALL_LIBDIR}"
    "/prefix/${CMAKE_INSTALL_INCLUDEDIR}")
configure_file(
    cmake/mcpp98-config.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/mcpp98-config.cmake"
    @ONLY)

install(TARGETS mcpp98 ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/mcpp98-config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mcpp98)
