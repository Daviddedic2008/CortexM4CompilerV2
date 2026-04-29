cmake_minimum_required(VERSION 3.10)

# Set project name and restrict to GNU C
project(GPC LANGUAGES C)

# Ensure strictly GNU C (C11 is standard for modern compilers)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Useful for compiler dev to catch alignment or syntax bugs early
add_compile_options(-Wall -Wextra)

add_executable(GPC
    src/main.c
	src/frontend/lexer/tokenizer.c
	src/frontend/parser/astGen.c
	src/helper/arenaAlloc.c
)
target_compile_options(${PROJECT_NAME} PRIVATE -O2)
target_link_options(${PROJECT_NAME} PRIVATE -s)
target_compile_options(${PROJECT_NAME} PRIVATE -fstack-usage)
target_compile_options(${PROJECT_NAME} PRIVATE -w)
