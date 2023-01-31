project(base)

file(GLOB_RECURSE LibBase_H
	*.h)
	
file(GLOB_RECURSE LibBase_HPP 
	*.hpp)
	
file(GLOB_RECURSE LibBase_CPP 
	*.cpp)
	
file(GLOB_RECURSE LibBase_CC
	*.cc)

file(GLOB_RECURSE LibBase_C
	*.c)

set(_DEP_BASE_LIB 
	easyloggingpp
	net_uv
	libuv
	sqliter
)


SET(ENABLE_MYSQL ON)
SET(ENABLE_PG OFF)
SET(ENABLE_SQLITE3 OFF)

if (ENABLE_MYSQL)
add_definitions(-DORMPP_ENABLE_MYSQL)
list(APPEND _DEP_BASE_LIB mysql)
endif()

if (ENABLE_SQLITE3)
add_definitions(-DORMPP_ENABLE_SQLITE3)
endif()

if (ENABLE_PG)
add_definitions(-DORMPP_ENABLE_PG)
endif()


# log tag
add_definitions(-DELPP_DEFAULT_LOGGER="Base")

# STATIC SHARED
add_library(${PROJECT_NAME} STATIC ${LibBase_H} ${LibBase_HPP} ${LibBase_CPP} ${LibBase_CC} ${LibBase_C})

target_link_libraries(
	${PROJECT_NAME}
	PUBLIC
	${_DEP_BASE_LIB}
)


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/3rd)
