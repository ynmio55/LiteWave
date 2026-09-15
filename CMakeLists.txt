cmake_minimum_required(VERSION 3.21)
project(LiteWave VERSION 0.9.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets PrintSupport WebEngineWidgets Network Concurrent)

qt_add_executable(LiteWave
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/AdBlocker.cpp
    src/AdBlocker.h
    resources.qrc
)

target_link_libraries(LiteWave PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::WebEngineWidgets
    Qt6::Network
    Qt6::Concurrent
)

if(WIN32)
    set_target_properties(LiteWave PROPERTIES WIN32_EXECUTABLE TRUE)
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

include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(ShieldTests
        tests/AdBlockerTests.cpp src/AdBlocker.cpp src/AdBlocker.h resources.qrc)
    target_include_directories(ShieldTests PRIVATE src)
    target_link_libraries(ShieldTests PRIVATE Qt6::Test Qt6::WebEngineCore Qt6::Network Qt6::Concurrent)
    if(MSVC)
        target_compile_options(ShieldTests PRIVATE /utf-8)
    endif()
    add_test(NAME shield-rules COMMAND ShieldTests)
    set_tests_properties(shield-rules PROPERTIES TIMEOUT 30)

    option(LITEWAVE_GUI_TESTS "Run WebEngine integration tests (display required)" OFF)
    if(LITEWAVE_GUI_TESTS)
        qt_add_executable(ShieldIntegration
            tests/ShieldIntegration.cpp src/MainWindow.cpp src/MainWindow.h
            src/AdBlocker.cpp src/AdBlocker.h resources.qrc)
        target_include_directories(ShieldIntegration PRIVATE src)
        target_link_libraries(ShieldIntegration PRIVATE Qt6::Test Qt6::Widgets Qt6::PrintSupport
            Qt6::WebEngineWidgets Qt6::Network Qt6::Concurrent)
        add_test(NAME shield-webengine COMMAND ShieldIntegration)
        set_tests_properties(shield-webengine PROPERTIES TIMEOUT 60)
    endif()
endif()
