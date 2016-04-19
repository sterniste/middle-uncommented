cmake_minimum_required(VERSION 3.0)

project(odbc_query CXX)

file(GLOB CC_FILES *.cc)

find_package(Boost 1.58.0 REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${CC_FILES})

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)

# Specify here the include directories exported by this library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME})

# Expose odbc_query's definitions to other subprojects through cache variable.
set(${PROJECT_NAME}_DEFINITIONS ${Boost_DEFINITIONS} CACHE INTERNAL "${PROJECT_NAME}: Definitions" FORCE)
# Expose odbc_query's public includes (including Boost transitively) to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
