cmake_minimum_required(VERSION 3.10.0)
project(SCOOM VERSION 0.1.5 LANGUAGES C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -fsanitize=address -fsanitize=undefined -fstack-protector -g -fno-omit-frame-pointer -O2 -D_FORTIFY_SOURCE=2")

add_subdirectory(lib/DSA)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} O3 -DNDEBUG")
endif()

# Important for Clang
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(include_dir include)
set(src
	main.c
	src/terminal.c
	src/render.c
	src/file.c
	src/rows.c
	src/core.c
	src/highlight.c
	src/config.c
	src/input.c
	src/buffer.c
	src/config.c
)

add_executable(SCOOM ${src})
target_include_directories(SCOOM PRIVATE ${include_dir})
target_link_libraries(SCOOM PRIVATE DSA)