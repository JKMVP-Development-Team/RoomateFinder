cmake_minimum_required(VERSION 3.10)
project(RoommateFinderBackend)

set(CMAKE_CXX_STANDARD 17)

option(DOCKER_BUILD "Build for Docker" OFF)


add_executable(roommateapp
    src/main.cpp
    src/Matcher.cpp
    src/DBManager.cpp
)

# Add local headers (Crow + Asio)
target_include_directories(roommateapp PRIVATE
    src
    include
    include/asio
    ${LIBMONGOCXX_INCLUDE_DIRS}
    ${LIBBSONCXX_INCLUDE_DIRS}
)


if(DOCKER_BUILD)
    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    target_link_libraries(roommateapp PRIVATE
        ${LIBMONGOCXX_LIBRARIES}
        ${LIBBSONCXX_LIBRARIES}
    )
else()
    find_package(bsoncxx CONFIG REQUIRED)
    target_link_libraries(roommateapp PRIVATE $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>)

    find_package(mongocxx CONFIG REQUIRED)
    target_link_libraries(roommateapp PRIVATE $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>)
endif()
