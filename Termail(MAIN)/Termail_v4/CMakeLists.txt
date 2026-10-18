cmake_minimum_required(VERSION 3.16)
project(Termail VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt6 依赖
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

# 源文件
set(SOURCES
    main.cpp
    ui/terminal_ui.cpp
    commands/commands.cpp
    commands/file_commands.cpp
    commands/network_commands.cpp
    commands/system_commands.cpp
    commands/text_commands.cpp
    commands/fun_commands.cpp
    utils/utils.cpp
    games/minesweeper.cpp
    customcmd/custom_commands_dialog.cpp
)

# 头文件
set(HEADERS
    config.h
    ui/terminal_ui.h
    commands/commands.h
    utils/utils.h
    games/minesweeper.h
    customcmd/custom_commands_dialog.h
)

# 资源文件
set(RESOURCES
    resources/custom_commands.json
)

# 可执行文件
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

qt_add_resources(${PROJECT_NAME} "termail_resources"
    PREFIX "/"
    FILES resources/custom_commands.json
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/games
    ${CMAKE_CURRENT_SOURCE_DIR}/customcmd
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Core
    Qt6::Gui
    Qt6::Network
)

# 平台判断
if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_APP_NAME "Termail终端"
        QT_ANDROID_PACKAGE_NAME "com.termail.terminal"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )
elseif(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    target_sources(${PROJECT_NAME} PRIVATE resources/termail.rc)
elseif(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "Termail终端"
        MACOSX_BUNDLE_BUNDLE_VERSION "4.0.0"
    )
endif()

# 安装规则
if(NOT ANDROID)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(FILES resources/custom_commands.json
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
    )
endif()