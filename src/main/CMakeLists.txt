cmake_minimum_required(VERSION 3.0)

project(main CXX)

add_compile_options(-DLEVEL_LOGGING)

file(GLOB CC_FILES *.cc)

# prefer static to dynamic libraries
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)

find_package(Boost 1.58.0 REQUIRED)

find_package(PkgConfig)
find_package(XercesC REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58.0 COMPONENTS log log_setup program_options filesystem system thread REQUIRED)
find_package(Threads REQUIRED)

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  find_library(ICUUC_LIBS sicuuc)
  find_library(ICUDATA_LIBS sicudt)
else ()
  find_library(ODBC_LIBS odbc)
  find_library(ICUUC_LIBS icuuc)
  find_library(ICUDATA_LIBS icudata)
endif ()

include_directories(${Boost_INCLUDE_DIRS})

add_executable(middle ${CC_FILES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  target_link_libraries(middle middle_app sqlsvr_ddl normal_ddl odbc_query universals ${XercesC_LIBRARY} ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ICUUC_LIBS} ${ICUDATA_LIBS} libstdc++.a libgcc_eh.a libodbc32.dll ${CMAKE_THREAD_LIBS_INIT})
else ()
  target_link_libraries(middle middle_app sqlsvr_ddl normal_ddl odbc_query universals ${XercesC_LIBRARY} ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_THREAD_LIBRARY} ${ODBC_LIBS} ${ICUUC_LIBS} ${ICUDATA_LIBS} ${CMAKE_THREAD_LIBS_INIT} -lltdl -ldl)  
endif ()
