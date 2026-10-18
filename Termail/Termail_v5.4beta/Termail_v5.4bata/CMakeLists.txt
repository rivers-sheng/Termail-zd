cmake_minimum_required(VERSION 3.16)
project(Termail VERSION 5.4.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

set(SOURCES
    main.cpp
    terminal/terminal.cpp
    commands/commands.cpp
    commands/file_commands.cpp
    commands/network_commands.cpp
    commands/system_commands.cpp
    commands/text_commands.cpp
    commands/fun_commands.cpp
    commands/cmenu.cpp
    games/minesweeper.cpp
    utils/utils.cpp
    commands/many1.cpp
    commands/many2.cpp
    commands/many3.cpp
    commands/log_commands.cpp
)

set(HEADERS
    config.h
    terminal/terminal.h
    commands/commands.h
    games/minesweeper.h
    utils/utils.h
)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

qt_add_resources(${PROJECT_NAME} "termail_resources"
    PREFIX "/"
    FILES
        resources/custom_commands.json
        LICENSE/GPL.txt
        resources/busybox/busybox
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/terminal
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/games
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network
)

if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_APP_NAME "Termail终端"
        QT_ANDROID_PACKAGE_NAME "com.termail.terminal"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()
