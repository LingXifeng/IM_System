cmake_minimum_required(VERSION 3.16)

project(IMServer)

set(CMAKE_CXX_STANDARD 17)

add_executable(imserver
        main.cpp
    )

find_package(Drogon CONFIG REQUIRED)

target_link_libraries(imserver PRIVATE Drogon::Drogon)