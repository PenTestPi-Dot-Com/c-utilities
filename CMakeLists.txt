cmake_minimum_required(VERSION 3.0)

add_compile_options(
	-Wall
	-Wextra
	-Wpedantic
	-O3
	-m64
	-mrdrnd
	-s
)

project(play)

add_executable(
	sqlfs
	sqlfs.c
)

target_link_libraries(
sqlfs
sqlite3
json-c
curl
)

add_custom_target(run
    COMMAND sqlfs
    DEPENDS sqlfs
    WORKING_DIRECTORY ${CMAKE_PROJECT_DIR}
)
