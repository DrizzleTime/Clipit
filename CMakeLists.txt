cmake_minimum_required(VERSION 3.21)

project(Clipit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(clipit_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(Qt6 6.8 REQUIRED COMPONENTS DBus)
endif()

qt_standard_project_setup(REQUIRES 6.8)

qt_add_library(clipit_core STATIC
    src/core/annotation.cpp
    src/core/annotation.h
    src/core/annotationrenderer.cpp
    src/core/annotationrenderer.h
    src/core/screenshotstorage.cpp
    src/core/screenshotstorage.h
)
target_include_directories(clipit_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(clipit_core PUBLIC Qt6::Core Qt6::Gui)
clipit_enable_warnings(clipit_core)

set(CLIPIT_APP_SOURCES
    src/app/applicationoptions.cpp
    src/app/applicationoptions.h
    src/app/main.cpp
    src/app/screenshotservice.cpp
    src/app/screenshotservice.h
    src/platform/clipboardservice.cpp
    src/platform/clipboardservice.h
    src/platform/qtscreencapturebackend.cpp
    src/platform/qtscreencapturebackend.h
    src/platform/screencapturebackend.h
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND CLIPIT_APP_SOURCES
        src/platform/portalscreencapturebackend.cpp
        src/platform/portalscreencapturebackend.h
    )
endif()

qt_add_executable(appClipit ${CLIPIT_APP_SOURCES})

qt_add_qml_module(appClipit
    URI Clipit
    VERSION 1.0
    QML_FILES
        Main.qml
        CaptureOverlay.qml
        components/AnnotationLayer.qml
        components/CaptureToolbar.qml
        components/ColorPalette.qml
        components/Glyph.qml
        components/Toast.qml
)

set_target_properties(appClipit PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER io.clipit.app
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_include_directories(appClipit PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app
)
target_link_libraries(appClipit PRIVATE clipit_core Qt6::Quick)
clipit_enable_warnings(appClipit)
target_compile_definitions(appClipit PRIVATE CLIPIT_VERSION="${PROJECT_VERSION}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(appClipit PRIVATE Qt6::DBus)
    target_compile_definitions(appClipit PRIVATE CLIPIT_HAS_DBUS_PORTAL)
endif()

include(GNUInstallDirs)
install(TARGETS appClipit
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(FILES LICENSE
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/Clipit
)

include(CTest)
if(BUILD_TESTING)
    add_test(
        NAME cli_help_without_display
        COMMAND ${CMAKE_COMMAND} -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
                --unset=QT_QPA_PLATFORM
                $<TARGET_FILE:appClipit> --help
    )
    set_tests_properties(cli_help_without_display PROPERTIES
        PASS_REGULAR_EXPRESSION "--region"
    )
    add_test(
        NAME cli_version_without_display
        COMMAND ${CMAKE_COMMAND} -E env --unset=DISPLAY --unset=WAYLAND_DISPLAY
                --unset=QT_QPA_PLATFORM
                $<TARGET_FILE:appClipit> --version
    )
    set_tests_properties(cli_version_without_display PROPERTIES
        PASS_REGULAR_EXPRESSION "Clipit ${PROJECT_VERSION}"
    )

    find_package(Qt6 6.8 QUIET COMPONENTS Test)
    if(TARGET Qt6::Test)
        qt_add_executable(tst_applicationoptions
            tests/tst_applicationoptions.cpp
            src/app/applicationoptions.cpp
            src/app/applicationoptions.h
        )
        target_include_directories(tst_applicationoptions PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        )
        target_link_libraries(tst_applicationoptions PRIVATE Qt6::Core Qt6::Test)
        add_test(NAME applicationoptions COMMAND tst_applicationoptions)

        qt_add_executable(tst_annotationrenderer
            tests/tst_annotationrenderer.cpp
        )
        target_link_libraries(tst_annotationrenderer PRIVATE clipit_core Qt6::Test)
        add_test(NAME annotationrenderer COMMAND tst_annotationrenderer)

        qt_add_executable(tst_screenshotstorage
            tests/tst_screenshotstorage.cpp
        )
        target_link_libraries(tst_screenshotstorage PRIVATE clipit_core Qt6::Test)
        add_test(NAME screenshotstorage COMMAND tst_screenshotstorage)

        qt_add_executable(tst_screenshotservice
            tests/tst_screenshotservice.cpp
            src/app/screenshotservice.cpp
            src/app/screenshotservice.h
            src/platform/clipboardservice.cpp
            src/platform/clipboardservice.h
            src/platform/qtscreencapturebackend.cpp
            src/platform/qtscreencapturebackend.h
            src/platform/screencapturebackend.h
        )
        target_include_directories(tst_screenshotservice PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        )
        target_link_libraries(tst_screenshotservice
            PRIVATE clipit_core Qt6::Quick Qt6::Test
        )
        add_test(NAME screenshotservice COMMAND tst_screenshotservice)
        set_tests_properties(screenshotservice PROPERTIES
            ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        )
    else()
        message(WARNING "Qt Test 未安装，跳过单元测试")
    endif()
endif()
