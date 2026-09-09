cmake_minimum_required(VERSION 3.21)

project(tsp VERSION 1.1.0 LANGUAGES C)

include(CTest)

option(TSP_ENABLE_SANITIZERS
       "Build the C core and tests with AddressSanitizer and UndefinedBehaviorSanitizer"
       OFF)
option(TSP_ENABLE_FNO_COMMON
       "Build the C core and tests with -fno-common"
       OFF)
option(TSP_ENABLE_CPP_CHECK
       "Build a C++17 translation unit that includes the public C API"
       OFF)
option(TSP_BUILD_DESKTOP
       "Build the optional C++ desktop adapter and its tests"
       OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(TSP_SANITIZER_FLAG -fsanitize=address,undefined)

function(tsp_enable_c_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
                -Wstrict-prototypes
                -Werror
        )
    endif()
endfunction()

function(tsp_enable_sanitizer_instrumentation target compiler_id)
    if(TSP_ENABLE_SANITIZERS)
        if(MSVC OR NOT "${compiler_id}" MATCHES "^(AppleClang|Clang|GNU)$")
            message(
                FATAL_ERROR
                "TSP_ENABLE_SANITIZERS requires AppleClang, Clang, or GCC with a GNU-compatible frontend"
            )
        endif()
        target_compile_options(
            ${target}
            PRIVATE
                -O1
                ${TSP_SANITIZER_FLAG}
                -fno-omit-frame-pointer
        )
    endif()
endfunction()

function(tsp_enable_c_verification target)
    tsp_enable_sanitizer_instrumentation(
        ${target}
        "${CMAKE_C_COMPILER_ID}"
    )

    if(TSP_ENABLE_FNO_COMMON)
        if(MSVC OR NOT CMAKE_C_COMPILER_ID MATCHES "^(AppleClang|Clang|GNU)$")
            message(
                FATAL_ERROR
                "TSP_ENABLE_FNO_COMMON requires AppleClang, Clang, or GCC with a GNU-compatible frontend"
            )
        endif()
        target_compile_options(${target} PRIVATE -fno-common)
    endif()
endfunction()

function(tsp_enable_cxx_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(
            ${target}
            PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
        )
    endif()
endfunction()

set(TSP_CORE_SOURCES
    src/graph.c
    src/heap.c
    src/solver.c
    src/status.c
)

add_library(tsp_core STATIC ${TSP_CORE_SOURCES})
add_library(tsp::core ALIAS tsp_core)
set_target_properties(tsp_core PROPERTIES OUTPUT_NAME tsp)
target_include_directories(
    tsp_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
tsp_enable_c_warnings(tsp_core)
tsp_enable_c_verification(tsp_core)

if(TSP_ENABLE_SANITIZERS)
    target_link_options(tsp_core INTERFACE ${TSP_SANITIZER_FLAG})
endif()

if(UNIX)
    target_link_libraries(tsp_core PUBLIC m)
endif()

add_executable(tsp_cli cli/main.c)
set_target_properties(tsp_cli PROPERTIES OUTPUT_NAME tsp)
target_link_libraries(tsp_cli PRIVATE tsp::core)
tsp_enable_c_warnings(tsp_cli)

if(BUILD_TESTING)
    add_executable(tsp_tests tests/tests.c)
    set_target_properties(tsp_tests PROPERTIES OUTPUT_NAME tsp-tests)
    target_include_directories(tsp_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    target_link_libraries(tsp_tests PRIVATE tsp::core)
    tsp_enable_c_warnings(tsp_tests)
    tsp_enable_c_verification(tsp_tests)

    add_test(NAME tsp-tests COMMAND tsp_tests)
    set_tests_properties(
        tsp-tests
        PROPERTIES WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

if(TSP_BUILD_DESKTOP OR TSP_ENABLE_CPP_CHECK)
    enable_language(CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

if(TSP_BUILD_DESKTOP)
    add_subdirectory(gui)
endif()

if(TSP_ENABLE_CPP_CHECK)
    file(
        GENERATE
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/cpp-header-check.cpp"
        CONTENT "#include <tsp/tsp.h>\nint main() { return 0; }\n"
    )
    add_executable(
        tsp_cpp_header_check
        EXCLUDE_FROM_ALL
        "${CMAKE_CURRENT_BINARY_DIR}/cpp-header-check.cpp"
    )
    target_include_directories(
        tsp_cpp_header_check
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    tsp_enable_cxx_warnings(tsp_cpp_header_check)
endif()
