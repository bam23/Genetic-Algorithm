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

    set(tsp_desktop_platform_sources)
    if(APPLE)
        set(tsp_macos_icon
            "${PROJECT_SOURCE_DIR}/resources/icons/tsp-app-icon.icns"
        )
        set_source_files_properties(
            "${tsp_macos_icon}"
            PROPERTIES MACOSX_PACKAGE_LOCATION Resources
        )
        list(APPEND tsp_desktop_platform_sources "${tsp_macos_icon}")
    elseif(WIN32)
        file(
            TO_CMAKE_PATH
            "${PROJECT_SOURCE_DIR}/resources/icons/tsp-app-icon.ico"
            TSP_WINDOWS_ICON_PATH
        )
        configure_file(
            "${PROJECT_SOURCE_DIR}/resources/windows/tsp-app.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/tsp-app.rc"
            @ONLY
        )
        set_property(
            SOURCE "${CMAKE_CURRENT_BINARY_DIR}/tsp-app.rc"
            APPEND
            PROPERTY OBJECT_DEPENDS
                     "${PROJECT_SOURCE_DIR}/resources/icons/tsp-app-icon.ico"
        )
        list(
            APPEND
            tsp_desktop_platform_sources
            "${CMAKE_CURRENT_BINARY_DIR}/tsp-app.rc"
        )
    endif()

    qt_add_executable(
        tsp_desktop
        WIN32
        MACOSX_BUNDLE
        convergence_view.cpp
        convergence_view.hpp
        main.cpp
        main_window.cpp
        main_window.hpp
        route_view.cpp
        route_view.hpp
        solver_worker.cpp
        solver_worker.hpp
        "${PROJECT_SOURCE_DIR}/resources/tsp_resources.qrc"
        ${tsp_desktop_platform_sources}
    )
    set_target_properties(
        tsp_desktop
        PROPERTIES
            AUTOMOC ON
            AUTORCC ON
            MACOSX_BUNDLE_BUNDLE_NAME "Traveling Salesman Problem"
            MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
            MACOSX_BUNDLE_GUI_IDENTIFIER "com.bam23.tsp"
            MACOSX_BUNDLE_ICON_FILE "tsp-app-icon.icns"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
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
