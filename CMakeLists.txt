cmake_minimum_required(VERSION 3.21)
project(LiteWave VERSION 1.0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets PrintSupport WebEngineWidgets WebEngineCore Network Concurrent)

qt_add_executable(LiteWave
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    resources.qrc
)

target_link_libraries(LiteWave PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::Concurrent
)

if(WIN32)
    set_target_properties(LiteWave PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(LiteWave PRIVATE assets/litewave.rc)
endif()

include(GNUInstallDirs)
install(TARGETS LiteWave RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/LiteWave.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/litewave.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME litewave.svg)

if(MSVC)
    target_compile_options(LiteWave PRIVATE /utf-8)
endif()

