cmake_minimum_required(VERSION 3.16)

project(IMServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Drogon CONFIG REQUIRED)

add_executable(imserver
    main.cpp

    controller/AuthController.cpp

    service/AuthService.cpp

    dao/UserDao.cpp
)

target_link_libraries(imserver
    PRIVATE
    Drogon::Drogon
)