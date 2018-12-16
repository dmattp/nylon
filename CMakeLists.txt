# The name of our project is "HELLO". CMakeLists files in this project can 
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and 
# to the root binary directory of the project as ${HELLO_BINARY_DIR}. 
# Useful variables: http://www.cmake.org/Wiki/CMake_Useful_Variables

# to build, do this:
# mkdir build; cd build; cmake .. && cmake --build . && cp ./src/syscore.so ../nylon/ && ctest


cmake_minimum_required (VERSION 2.6) 

project (NYLON) 
set(NYLON_VERSION_MAJOR 0)
set(NYLON_VERSION_MINOR 1)

add_subdirectory (src) 
#add_subdirectory (tests)

# install rule for header files
#INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/nlog" DESTINATION include)

enable_testing()
add_test( NAME all-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND lua run-all-tests.lua)
