cmake_minimum_required(VERSION 3.8)

project(lua)

add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_LIST_DIR}/src/onelua.cpp
)

add_definitions(-DONELUA_MAKE_LIB)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)