cmake_minimum_required(VERSION 3.10)
project(RoommateFinderBackend)

set(CMAKE_CXX_STANDARD 17)

add_executable(roommateapp
    src/main.cpp
)

# Add local headers (Crow + Asio)
target_include_directories(roommateapp PRIVATE
    src
    include
    include/asio
)
