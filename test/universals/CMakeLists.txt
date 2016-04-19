cmake_minimum_required(VERSION 3.0)

project(test-universals CXX)

file(GLOB CC_FILES *.cc)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.58.0)

include_directories(${Boost_INCLUDE_DIRS})

add_executable(test-universals ${CC_FILES})

target_link_libraries(test-universals universals libgtest libgmock)

add_test(NAME test-universals COMMAND test-universals)
