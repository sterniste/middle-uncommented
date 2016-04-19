cmake_minimum_required(VERSION 3.0)

project(sqlsvr_ddl CXX)

add_compile_options(-DLEVEL_LOGGING -DFAKE_SQLSERVER -UDUMP_QUERY_ROWS_TO_VERBOSE)

file(GLOB CC_FILES *.cc)

include_directories(${Boost_INCLUDE_DIRS} ${universals_INCLUDE_DIRS} ${odbc_query_INCLUDE_DIRS} ${normal_ddl_INCLUDE_DIRS})

add_library(${PROJECT_NAME} STATIC ${CC_FILES})

# Required on Unix OS family to be able to be linked into shared libraries.
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE OFF)

# Specify here the include directories exported by this library
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME})

# Expose definitions to other subprojects through cache variable.
set(${PROJECT_NAME}_DEFINITIONS ${Boost_DEFINITIONS} ${universals_DEFINITIONS} ${odbc_query_DEFINITIONS} ${normal_ddl_DEFINITIONS} CACHE INTERNAL "${PROJECT_NAME}: Definitions" FORCE)
# Expose public includes (including odbc_query, normal_ddl transitively) to other subprojects through cache variable.
set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS} ${universals_INCLUDE_DIRS} ${odbc_query_INCLUDE_DIRS} ${normal_ddl_INCLUDE_DIRS} CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
