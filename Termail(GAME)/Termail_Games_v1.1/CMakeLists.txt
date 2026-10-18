cmake_minimum_required(VERSION 3.16)
project(TermailGames VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

set(SOURCES
    main.cpp
    terminal/terminal.cpp
    commands/commands.cpp
    commands/help_commands.cpp
    commands/fun_commands.cpp
    htmlgames/htmlgames.cpp
    utils/utils.cpp
)

set(HEADERS
    config.h
    terminal/terminal.h
    commands/commands.h
    htmlgames/htmlgames.h
    utils/utils.h
)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

qt_add_resources(${PROJECT_NAME} "termail_games_resources"
    PREFIX "/"
    FILES
        LICENSE/GPL.txt
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/terminal
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/htmlgames
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui
)

if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_APP_NAME "Termail Games"
        QT_ANDROID_PACKAGE_NAME "com.termail.spical"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
endif()
