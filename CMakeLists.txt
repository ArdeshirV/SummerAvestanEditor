cmake_minimum_required(VERSION 3.20)
project(AvestanEditor VERSION 1.0 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(AvestanEditor
    src/main.cpp
    src/mainwindow.cpp
    src/dialogs.cpp
    src/config.cpp
    src/document.cpp
    src/editor.cpp
    src/keyboard.cpp
    src/theme.cpp
    ${CMAKE_SOURCE_DIR}/windows_icon.rc
)

target_link_libraries(AvestanEditor PRIVATE Qt6::Widgets)

qt6_add_resources(AVESTAN_RESOURCES
    "${CMAKE_SOURCE_DIR}/resources.qrc"
)

target_sources(AvestanEditor PRIVATE "${AVESTAN_RESOURCES}")
