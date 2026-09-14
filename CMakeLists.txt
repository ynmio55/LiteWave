cmake_minimum_required(VERSION 3.21)
project(LiteWave VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets WebEngineWidgets)

qt_add_executable(LiteWave
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
)

target_link_libraries(LiteWave PRIVATE
    Qt6::Widgets
    Qt6::WebEngineWidgets
)

if(WIN32)
    set_target_properties(LiteWave PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
