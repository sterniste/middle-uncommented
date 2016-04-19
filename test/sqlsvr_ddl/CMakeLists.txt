cmake_minimum_required(VERSION 3.0)

project(test-sqlsvr_ddl CXX)

file(GLOB CC_FILES *.cc)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58.0 COMPONENTS log log_setup system thread REQUIRED)
find_package(Threads REQUIRED)

if (NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    find_library(ODBC_LIBS odbc)
endif ()

include_directories(${Boost_INCLUDE_DIRS})

add_executable(test-sqlsvr_ddl ${CC_FILES})

if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    target_link_libraries(test-sqlsvr_ddl sqlsvr_ddl normal_ddl odbc_query universals ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_SYSTEM_LIBRARY} libgtest libgmock ${Boost_THREAD_LIBRARY} libodbc32.dll ${CMAKE_THREAD_LIBS_INIT})
else ()
    target_link_libraries(test-sqlsvr_ddl sqlsvr_ddl normal_ddl odbc_query universals ${Boost_LOG_LIBRARY} ${Boost_LOG_SETUP_LIBRARY} ${Boost_SYSTEM_LIBRARY} libgtest libgmock ${Boost_THREAD_LIBRARY} ${ODBC_LIBS} ${CMAKE_THREAD_LIBS_INIT} -lltdl -ldl)
endif ()

add_test(NAME test-sqlsvr_ddl COMMAND test-sqlsvr_ddl)
