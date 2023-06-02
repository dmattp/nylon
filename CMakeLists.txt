# The name of our project is "HELLO". CMakeLists files in this project can 
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and 
# to the root binary directory of the project as ${HELLO_BINARY_DIR}. 
# Useful variables: http://www.cmake.org/Wiki/CMake_Useful_Variables

# to build, do this:
# mkdir build; cd build; cmake .. && cmake --build . && cp ./src/syscore.so ../nylon/ && ctest


cmake_minimum_required (VERSION 3.0) 

project (NYLON) 
set(NYLON_VERSION_MAJOR 0)
set(NYLON_VERSION_MINOR 1)

set(NYLON_INSTALL_SHARE "/usr/share/lua/5.4")
set(NYLON_INSTALL_LIB   "/usr/lib/lua/5.4")

set(LUABIND_BUILD_SHARED ON)

if(NOT LUA_FOUND AND NOT LUA54_FOUND)
	find_package(Lua54 REQUIRED)
	set(LUA_INCLUDE_DIRS "${LUA_INCLUDE_DIR}")
endif()


add_subdirectory(luabind-deboostified)
add_subdirectory (src) 

#add_subdirectory (tests)
# install rule for header files
#INSTALL(FILES     "${CMAKE_CURRENT_SOURCE_DIR}/include/nlog.h" DESTINATION include)
#INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/nlog" DESTINATION include)

enable_testing()
add_test( NAME all-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND lua run-all-tests.lua)

INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/debug.lua" DESTINATION "${NYLON_INSTALL_SHARE}/nylon/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/core.lua" DESTINATION  "${NYLON_INSTALL_SHARE}/nylon/")
INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/scheduler.lua" DESTINATION "${NYLON_INSTALL_SHARE}/nylon/")
