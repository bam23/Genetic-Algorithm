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

if(TSP_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

    add_executable(
        tsp_desktop
        main.cpp
        main_window.cpp
        main_window.hpp
        solver_worker.cpp
        solver_worker.hpp
    )
    set_target_properties(
        tsp_desktop
        PROPERTIES
            AUTOMOC ON
            OUTPUT_NAME tsp-desktop
    )
    target_link_libraries(
        tsp_desktop
        PRIVATE
            tsp_desktop_adapter
            Qt6::Core
            Qt6::Gui
            Qt6::Widgets
    )
    target_compile_features(tsp_desktop PRIVATE cxx_std_17)
    tsp_enable_cxx_warnings(tsp_desktop)
    tsp_enable_sanitizer_instrumentation(
        tsp_desktop
        "${CMAKE_CXX_COMPILER_ID}"
    )

    add_custom_command(
        TARGET tsp_desktop
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_SOURCE_DIR}/cities.dat"
            "$<TARGET_FILE_DIR:tsp_desktop>/cities.dat"
        VERBATIM
    )
endif()
