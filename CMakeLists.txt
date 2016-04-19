cmake_minimum_required(VERSION 3.0)

project(middle)

# use local cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} cmake/Modules)

enable_testing()

set(CMAKE_VERBOSE_MAKEFILE ON)

# remove 'lib' prefix for shared libraries on Windows
if (WIN32)
  set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif ()

add_subdirectory(src)
add_subdirectory(test)








