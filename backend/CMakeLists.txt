cmake_minimum_required(VERSION 3.10)
project(RoommateFinderBackend)

set(CMAKE_CXX_STANDARD 17)

option(DOCKER_BUILD "Build for Docker" OFF)


add_executable(roommateapp
    src/main.cpp
)

# Add local headers (Crow + Asio)
target_include_directories(roommateapp PRIVATE
    src
    include
    include/asio
    C:/vcpkg/installed/x64-windows/include
)

find_package(mongocxx REQUIRED CONFIG)
find_package(bsoncxx REQUIRED CONFIG)

target_link_libraries(roommateapp PRIVATE
    mongo::mongocxx_shared
    mongo::bsoncxx_shared
)
