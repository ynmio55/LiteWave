cmake_minimum_required(VERSION 3.21)
project(LiteWave VERSION 1.1.0 LANGUAGES CXX)
find_program(CARGO_EXECUTABLE cargo REQUIRED)
set(LITEWAVE_RUST_TARGET ${CMAKE_CURRENT_BINARY_DIR}/rust-target)
if(WIN32)
  set(LITEWAVE_ENGINE_LIB ${LITEWAVE_RUST_TARGET}/release/litewave_adblock_engine.lib)
else()
  set(LITEWAVE_ENGINE_LIB ${LITEWAVE_RUST_TARGET}/release/liblitewave_adblock_engine.a)
endif()
add_custom_command(OUTPUT ${LITEWAVE_ENGINE_LIB}
  COMMAND ${CMAKE_COMMAND} -E env CARGO_TARGET_DIR=${LITEWAVE_RUST_TARGET} ${CARGO_EXECUTABLE} build --manifest-path ${CMAKE_CURRENT_SOURCE_DIR}/shield_engine/Cargo.toml --release
  DEPENDS shield_engine/Cargo.toml shield_engine/src/lib.rs shield_engine/filters/easylist_general_block.txt shield_engine/filters/easylist_adservers.txt)
add_custom_target(LiteWaveFilterEngineBuild DEPENDS ${LITEWAVE_ENGINE_LIB})
add_library(LiteWaveFilterEngine STATIC IMPORTED GLOBAL)
set_target_properties(LiteWaveFilterEngine PROPERTIES IMPORTED_LOCATION ${LITEWAVE_ENGINE_LIB})
add_dependencies(LiteWaveFilterEngine LiteWaveFilterEngineBuild)

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
    src/AdBlocker.cpp
    src/AdBlocker.h
    src/LiteWaveFilterEngine.h
    src/search/SearchEngineManager.cpp
    src/search/SearchEngineManager.h
    src/ui/FindBar.cpp
    src/ui/FindBar.h
    src/ui/SearchSuggestionPopup.cpp
    src/ui/SearchSuggestionPopup.h
    resources.qrc
)

target_include_directories(LiteWave PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/search ${CMAKE_CURRENT_SOURCE_DIR}/src/ui)

execute_process(
    COMMAND git rev-parse HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE LITEWAVE_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT LITEWAVE_GIT_COMMIT)
    set(LITEWAVE_GIT_COMMIT "dev")
endif()

target_compile_definitions(LiteWave PRIVATE
    LITEWAVE_APP_VERSION="${PROJECT_VERSION}"
    LITEWAVE_GIT_COMMIT="${LITEWAVE_GIT_COMMIT}"
)

add_dependencies(LiteWave LiteWaveFilterEngineBuild)
target_link_libraries(LiteWave PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::Concurrent
    LiteWaveFilterEngine
)

if(WIN32)
    target_link_libraries(LiteWave PRIVATE ws2_32 userenv bcrypt ntdll advapi32 shell32 ole32 oleaut32)
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
install(FILES assets/litewave.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps
        RENAME litewave.png)

if(MSVC)
    target_compile_options(LiteWave PRIVATE /utf-8)
endif()


include(CTest)
if(BUILD_TESTING)
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    qt_add_executable(ShieldTests
        tests/ShieldTests.cpp
        src/AdBlocker.cpp
        src/AdBlocker.h
        src/search/SearchEngineManager.cpp
        src/search/SearchEngineManager.h
    )
    target_include_directories(ShieldTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/search)
    target_link_libraries(ShieldTests PRIVATE
        Qt6::Test
        Qt6::WebEngineCore
        LiteWaveFilterEngine
    )
    if(WIN32)
        target_link_libraries(ShieldTests PRIVATE ws2_32 userenv bcrypt ntdll advapi32 shell32 ole32 oleaut32)
    endif()
    add_dependencies(ShieldTests LiteWaveFilterEngineBuild)
    add_test(NAME ShieldTests COMMAND ShieldTests)
endif()
