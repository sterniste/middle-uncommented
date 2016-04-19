cmake_minimum_required(VERSION 3.0)

project(test-middle_app CXX)

file(GLOB CC_FILES *.cc)

find_package(PkgConfig)
find_package(XercesC REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58.0 COMPONENTS log log_setup system thread REQUIRED)
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

add_executable(test-middle_app ${CC_FILES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(test-middle_app middle_app sqlsvr_ddl normal_ddl odbc_query universals ${XercesC_LIBRARY} ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_SYSTEM_LIBRARY} libgtest libgmock ${Boost_THREAD_LIBRARY} libodbc32.dll ${CMAKE_THREAD_LIBS_INIT})
else ()
    target_link_libraries(test-middle_app middle_app sqlsvr_ddl normal_ddl odbc_query universals ${XercesC_LIBRARY} ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_SYSTEM_LIBRARY} libgtest libgmock ${Boost_THREAD_LIBRARY} ${ODBC_LIBS} ${CMAKE_THREAD_LIBS_INIT} -lltdl -ldl)
endif ()

add_test(NAME test-middle_app COMMAND test-middle_app)
