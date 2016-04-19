cmake_minimum_required(VERSION 3.0)

project(src)

# iwyu: "include-what-you-use" suggests include/forward declaration optimizations during compilation
if (BUILD_IWYU)
  include(cmake/Modules/IWYU.cmake)
  iwyu_enable()
  find_program(IWYU_PATH NAMES include-what-you-use iwyu)
  if (NOT IWYU_PATH)
    message(FATAL_ERROR "include-what-you-use (iwyu) not found")
  endif ()
endif ()

# once for all subprojects
add_compile_options(-std=c++0x -Wall)

add_subdirectory(universals) # a static library (depends on INCLUDES: Boost)
add_subdirectory(odbc_query) # a static library (depends on INCLUDES: Boost)
add_subdirectory(normal_ddl) # a static library (depends on INCLUDES: Boost, universals)
add_subdirectory(sqlsvr_ddl) # a static library (depends on INCLUDES: Boost, universals, odbc_query, normal_ddl)
add_subdirectory(middle_app) # a static library (depends on INCLUDES: Xerces-C, Boost, universals, normal_ddl, sqlsvr_ddl)

add_subdirectory(main) # middle executable (depends on INCLUDES: Boost, sqlsvr_ddl, user_impls; LIBS: ICU, Xerces-C, Boost, universals, odbc_query, normal_ddl, sqlsvr_ddl, middle_app)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if (DOXYGEN_FOUND)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../doc/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif ()








