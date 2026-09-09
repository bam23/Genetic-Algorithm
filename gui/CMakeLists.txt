add_library(
    tsp_desktop_adapter
    STATIC
        solver_adapter.cpp
        solver_adapter.hpp
        solver_types.hpp
)
target_include_directories(
    tsp_desktop_adapter
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(tsp_desktop_adapter PUBLIC tsp::core)
target_compile_features(tsp_desktop_adapter PUBLIC cxx_std_17)
tsp_enable_cxx_warnings(tsp_desktop_adapter)
tsp_enable_sanitizer_instrumentation(
    tsp_desktop_adapter
    "${CMAKE_CXX_COMPILER_ID}"
)

if(BUILD_TESTING)
    add_executable(tsp_adapter_tests adapter_tests.cpp)
    target_link_libraries(tsp_adapter_tests PRIVATE tsp_desktop_adapter)
    target_compile_features(tsp_adapter_tests PRIVATE cxx_std_17)
    tsp_enable_cxx_warnings(tsp_adapter_tests)
    tsp_enable_sanitizer_instrumentation(
        tsp_adapter_tests
        "${CMAKE_CXX_COMPILER_ID}"
    )

    add_test(NAME tsp-adapter-tests COMMAND tsp_adapter_tests)
    set_tests_properties(
        tsp-adapter-tests
        PROPERTIES WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
endif()
