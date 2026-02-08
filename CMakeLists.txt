# default version for ubuntu 24.04
cmake_minimum_required(VERSION 3.28.3)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)
set(APP_VERSION_PATCH 0)

project(phtread-levenshtein
        VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED OFF)

include(FetchContent)

include(cmake/gtest_conf.cmake)
include(CTest)

include(cmake/boilerplate.cmake)
include(cmake/paths.cmake)

add_subdirectory(src)
