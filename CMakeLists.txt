# Based on: https://gitlab.com/CLIUtils/modern-cmake

cmake_minimum_required(VERSION 3.28)
project(
        hage
        VERSION 0.1
        DESCRIPTION "A collection of utilities I have created"
        LANGUAGES CXX)

include(CheckCXXCompilerFlag)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)


    CHECK_CXX_COMPILER_FLAG("-march=native" COMPILER_SUPPORTS_MARCH_NATIVE)
    if(COMPILER_SUPPORTS_MARCH_NATIVE)
        add_compile_options("-march=native")
    endif()

    include(CTest)
endif()



include(FetchContent)

add_subdirectory(src)
add_subdirectory(tests)
