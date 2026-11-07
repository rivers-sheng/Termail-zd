cmake_minimum_required(VERSION 3.16)
project(Termail VERSION 6.5.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui Network)

# Lua 子目录（用 C 编译器编译）
add_subdirectory(lua)

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
    commands/many1.cpp
    commands/many2.cpp
    commands/many3.cpp
    commands/many4.cpp
    commands/log_commands.cpp
    commands/proot_commands.cpp
    commands/session_commands.cpp
    commands/network2_commands.cpp
    commands/ssh_commands.cpp
    commands/about_commands.cpp
    commands/config_commands.cpp
    commands/enhanced_commands.cpp
    commands/notify_commands.cpp
    editor/editor_dialog.cpp
    editor/editor_commands.cpp
    lua/lua_engine.cpp
    commands/lua_commands.cpp
    utils/utils.cpp
)

set(HEADERS
    config.h
    terminal/terminal.h
    commands/commands.h
    editor/editor_dialog.h
    lua/lua_engine.h
    utils/utils.h
)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

qt_add_resources(${PROJECT_NAME} "termail_resources"
    PREFIX "/"
    FILES
        resources/custom_commands.json
        resources/busybox/busybox
        resources/proot/proot
        resources/proot/alpine.tar
        LICENSE/GPL.txt
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/terminal
    ${CMAKE_CURRENT_SOURCE_DIR}/commands
    ${CMAKE_CURRENT_SOURCE_DIR}/editor
    ${CMAKE_CURRENT_SOURCE_DIR}/lua
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets Qt6::Core Qt6::Gui Qt6::Network
    lua
)

if(ANDROID)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        QT_ANDROID_APP_NAME "Termail"
        QT_ANDROID_PACKAGE_NAME "com.termail.terminal"
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
    )

    # =========================================================
    # 强行把 busybox 复制到 libs 目录并改名为 .so，绕过 Gradle 过滤
    # =========================================================
    set(BUSYBOX_SRC "${CMAKE_CURRENT_SOURCE_DIR}/resources/busybox/busybox")
    # Qt 默认编译输出库的目录通常在这个位置，Gradle 会扫描这个目录下的 .so
    set(ANDROID_LIBS_DIR "${CMAKE_CURRENT_BINARY_DIR}/android-build/libs/arm64-v8a")
    set(BUSYBOX_DST "${ANDROID_LIBS_DIR}/libbusybox.so")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ANDROID_LIBS_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${BUSYBOX_SRC}" "${BUSYBOX_DST}"
        COMMENT "Force packaging busybox as libbusybox.so"
    )
endif()
