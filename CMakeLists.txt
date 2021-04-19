cmake_minimum_required(VERSION 3.17)
project(mandelbrot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SFML_STATIC_LIBRARIES TRUE)
set(SFML_DIR "C:/SFML-2.5.1/lib/cmake/SFML")
find_package(SFML 2.5 COMPONENTS graphics REQUIRED)

add_executable(mandelbrot Main_basic.cpp)
target_compile_options(mandelbrot PRIVATE /std:c++latest)
target_compile_definitions(mandelbrot PRIVATE _CRT_SECURE_NO_WARNINGS)
target_sources(mandelbrot PRIVATE Main_basic.cpp)
target_link_libraries(mandelbrot PRIVATE sfml-graphics)
