cmake_minimum_required(VERSION 3.21)
project(AvestanEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(AVESTAN_ASSETS
    assets/app.json
    assets/languages.json
    assets/readings.json
    assets/unicode_names.tsv
    assets/unicode_pronunciations.tsv
    assets/UNICODE-NAMES-SOURCE.txt
    assets/fonts/UnicodeScriptFallback.ttf
    assets/fonts/FONT-SOURCES.txt
    assets/fonts/LICENSE.txt
    assets/ethereum-donation.png
    Icon.png
)

add_library(avestan-core STATIC
    src/config.cpp
    src/document.cpp
    src/editor.cpp
    src/keyboard.cpp
    src/theme.cpp
)
target_include_directories(avestan-core PUBLIC src)
target_link_libraries(avestan-core PUBLIC Qt6::Widgets)

add_executable(avestan-editor
    src/main.cpp
    src/mainwindow.cpp
)
target_link_libraries(avestan-editor PRIVATE avestan-core)

qt_add_resources(avestan-editor "avestan_assets"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${AVESTAN_ASSETS}
)

enable_testing()
add_subdirectory(tests)
