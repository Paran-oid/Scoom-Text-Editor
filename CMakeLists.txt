cmake_minimum_required(VERSION 3.10.0)
project(SCOOM VERSION 0.1.0 LANGUAGES C)

# important for Clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(include_dir include)
set(
	src 
	main.c
	src/terminal.c
	src/inout.c
	src/file.c
	src/operations.c
	src/core.c
)

add_executable(SCOOM ${src})
target_include_directories(SCOOM PRIVATE ${include_dir})
