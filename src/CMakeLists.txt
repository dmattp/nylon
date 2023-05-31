# Create a library called "syscore" which includes the source file "hello.cxx". 
# The extension is already found. Any number of sources could be listed here. 

message("LUA_LIBRARIES = ${LUA_LIBRARIES}")

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../nylon")

set(LUABIND_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/luabind-deboostified)

# link directories must be specified prior to the library; see http://stackoverflow.com/questions/8190103/cmake-library-not-linking

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   SET(LUA_LIBRARIES "lua54")
   include_directories( ./win ${LUA_INCLUDE_DIR} ${LUABIND_INCLUDE_DIR}  )  # Customize for your location of lua and luabind headers
   add_definitions( -DUNICODE=1 -DNOMINMAX=1 )
   link_directories( "c:/usr/lib" ) 
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  execute_process( COMMAND pkg-config --cflags glib-2.0 OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBCFLAGS )
  execute_process( COMMAND pkg-config --libs glib-2.0   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBLDFLAGS )
  # SET ( CMAKE_CXX_FLAGS "${GLIBCFLAGS} --std=c++0x" )
  include_directories( ./linux-gtk ${LUA_INCLUDE_DIR} ${LUABIND_INCLUDE_DIR} )  # Customize for your location of lua and luabind headers
endif()

add_library (syscore SHARED 
  NylonSysCore.cpp
)

# target_compile_features(syscore PUBLIC cxx_std_17)
set_target_properties(syscore PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED ON)


#set_target_properties(syscore PROPERTIES COMPILE_FLAGS "-save-temps") 

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries( syscore ${LUA_LIBRARIES} luabind )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries( syscore ${LUA_LIBRARIES} luabind rt "${GLIBLDFLAGS}")
endif()

# For lua extensions, we don't want it to be "libsyscore.so" but just "syscore.so"
# http://stackoverflow.com/questions/1564696/cmake-output-name-for-dynamic-loaded-library
set_target_properties( syscore PROPERTIES PREFIX "")

#if ( MSVC )
#   # http://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
##    set_target_properties( ${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${youroutputdirectory} )
#    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
#    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
#endif ( MSVC )


