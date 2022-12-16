cmake_minimum_required(VERSION 3.8)

project(mysql)


add_library(${PROJECT_NAME} STATIC
    ${CMAKE_CURRENT_LIST_DIR}/empty.cpp
)

if(WIN32)
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/libmariadb.lib
)
else()
    # apt install libmariadb-dev
target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	mariadb
	pthread
)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
