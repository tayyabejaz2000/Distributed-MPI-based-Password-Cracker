cmake_minimum_required(VERSION 3.11)
project(project)

set(CMAKE_CXX_COMPILER "mpic++")

set(CXX_STANDARD "c++17")

set(CMAKE_CXX_FLAGS "-fopenmp")

add_compile_options(
    -fno-diagnostics-show-option -fstack-protector-all
    -g -O2 -std=c++17 
    -Wall -Wfatal-errors
    -Wextra -Wpedantic -Walloc-zero -Walloca
    -Wctor-dtor-privacy -Wduplicated-cond -Wduplicated-branches
    -Werror -Wextra-semi -Winit-self -Wlogical-op
    -Wold-style-cast -Wshadow -Wunused-const-variable=1
    -Wzero-as-null-pointer-constant
)

link_libraries(-lcrypt)
# Create the executable from the source file main.cpp:
add_executable(project src/main.cpp)