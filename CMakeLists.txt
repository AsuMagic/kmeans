cmake_minimum_required(VERSION 2.8)

project(kmeans)
add_executable(${PROJECT_NAME} "main.cpp")

set(CMAKE_CXX_FLAGS "-std=c++1z")

target_link_libraries(kmeans sfml-system sfml-graphics sfml-window)
