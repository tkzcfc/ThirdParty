cmake_minimum_required(VERSION 3.8)

project(rapidjson)


add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_LIST_DIR}/empty.cpp
)


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})