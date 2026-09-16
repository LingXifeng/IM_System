cmake_minimum_required(VERSION 3.19)

project(client LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network WebSockets)

qt_standard_project_setup()

set(OPENSSL_ROOT_DIR
    "D:/cs_tool/QT6/Tools/mingw1310_64/opt"
)

set(OPENSSL_INCLUDE_DIR
    "${OPENSSL_ROOT_DIR}/include"
)

set(OPENSSL_CRYPTO_LIBRARY
    "${OPENSSL_ROOT_DIR}/lib/libcrypto.dll.a"
)

qt_add_executable(client
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    chatwindow.h
    chatwindow.cpp
    chatwindow.ui
    network/websocketclient.h
    network/websocketclient.cpp
    crypto/cryptomanager.h
    crypto/cryptomanager.cpp
    crypto/CryptoConfig.h
)

target_include_directories(client
    PRIVATE
        ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(client
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::WebSockets
        ${OPENSSL_CRYPTO_LIBRARY}
)

install(TARGETS client
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET client
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})