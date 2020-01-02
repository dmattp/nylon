# The name of our project is "HELLO". CMakeLists files in this project can 
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and 
# to the root binary directory of the project as ${HELLO_BINARY_DIR}. 
# Useful variables: http://www.cmake.org/Wiki/CMake_Useful_Variables

# to build, do this:
# mkdir build; cd build; cmake .. && cmake --build . && cp ./src/syscore.so ../nylon/ && ctest
cmake_minimum_required (VERSION 3.10) 
set(CMAKE_GENERATOR_TOOLSET v141_xp)

project (NYLON) 
set(NYLON_VERSION_MAJOR 0)
set(NYLON_VERSION_MINOR 1)

#add_subdirectory (tests)

# install rule for header files
#INSTALL(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/nlog" DESTINATION include)


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  set(LUA_ROOT "c:/opt/lua53")
  set(NAME_LIBLUA "liblua53")
  set(LUA_LIBRARIES "${LUA_ROOT}/lib/${NAME_LIBLUA}.lib")
  set(LUA_INCLUDE_DIR "${LUA_ROOT}/src")
else()
endif()



set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules" ${CMAKE_MODULE_PATH})
include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE ${CMAKE_BINARY_DIR}/ExtProjects)
get_directory_property(EXT_BASE_PATH EP_BASE)
set(EXT_INSTALL_PATH ${EXT_BASE_PATH}/Install)


include(luabind)
include(nylon)


enable_testing()
add_test( NAME all-tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/tests COMMAND lua run-all-tests.lua)


INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/debug.lua" DESTINATION /usr/share/lua/5.3/nylon)
INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/core.lua" DESTINATION /usr/share/lua/5.3/nylon)
INSTALL(FILES "${CMAKE_SOURCE_DIR}/nylon/scheduler.lua" DESTINATION /usr/share/lua/5.3/nylon)

