cmake_minimum_required(VERSION 3.17)
project(RayTracer C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options("$<$<CONFIG:DEBUG>:-DDEBUG>") #define DEBUG

add_subdirectory(lib)

add_executable(RayTracer src/main.cpp src/Engine/Shader.cpp src/Engine/Shader.hpp src/Engine/GL.hpp src/Engine/ShaderProgram.hpp)

target_link_libraries(RayTracer PUBLIC glad glfw glm)

target_include_directories(RayTracer PUBLIC src)