cmake_minimum_required(VERSION 3.21)
project(MasscanGUI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

add_executable(MasscanGUI WIN32
    src/main.cpp
    src/core/geo.hpp
    src/core/geo.cpp
    src/core/command_builder.hpp
    src/core/command_builder.cpp
    src/core/scanner_process.hpp
    src/core/scanner_process.cpp
    src/ui/collapsible_section.hpp
    src/ui/collapsible_section.cpp
    src/ui/fluent_scroll_bar.hpp
    src/ui/fluent_scroll_bar.cpp
    src/ui/fluent_tooltip.hpp
    src/ui/fluent_tooltip.cpp
    src/ui/badge_widget.hpp
    src/ui/badge_widget.cpp
    src/ui/main_window.hpp
    src/ui/main_window.cpp
    src/ui/theme.hpp
    src/ui/theme.cpp
    src/ui/theme_toggle.hpp
    src/ui/card_widget.hpp
    src/ui/item_delegates.hpp
    src/ui/high_hz_animator.hpp
    src/ui/hamburger_button.hpp
    src/ui/jelly_checkbox.hpp
    src/ui/glitch_button.hpp
    src/ui/github_button.hpp
    src/ui/terminal_box.hpp
    src/ui/icons.hpp
    resources/app.rc
    resources/resources.qrc
)

target_include_directories(MasscanGUI PRIVATE src)
target_link_libraries(MasscanGUI PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::Svg)
target_compile_definitions(MasscanGUI PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)

# In static Qt builds, link ONLY the 100% vital plugins and exclude all unnecessary plugins
qt_import_plugins(MasscanGUI
    NO_DEFAULT
    INCLUDE
        Qt6::QWindowsIntegrationPlugin
        Qt6::QSvgIconPlugin
        Qt6::QICOPlugin
        Qt6::QSvgPlugin
        Qt6::QSchannelBackendPlugin
)

if (MSVC)
    target_link_options(MasscanGUI PRIVATE "/ENTRY:mainCRTStartup")
elseif (MINGW)
    target_compile_options(MasscanGUI PRIVATE -Os -ffunction-sections -fdata-sections)
    target_link_options(MasscanGUI PRIVATE -static -static-libgcc -static-libstdc++ -Wl,--gc-sections -s)
endif()

if (WIN32)
    add_custom_command(
        TARGET MasscanGUI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/data"
            "$<TARGET_FILE_DIR:MasscanGUI>/data"
    )
endif()
