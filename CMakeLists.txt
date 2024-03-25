# Based on: https://gitlab.com/CLIUtils/modern-cmake

cmake_minimum_required(VERSION 3.28)
project(
        hage
        VERSION 0.1
        DESCRIPTION "A collection of utilities I have created"
        LANGUAGES CXX)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_EXTENSIONS OFF)

    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    include(CTest)
endif()

add_subdirectory(src)
