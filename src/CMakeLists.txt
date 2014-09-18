# Create a library called "syscore" which includes the source file "hello.cxx". 
# The extension is already found. Any number of sources could be listed here. 


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../nylon")

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   include_directories( c:/usr/share/lua523/include c:/m/home/pkg/boost_1_55_0 )  # Customize for your location of lua and luabind headers
   include_directories( ./win ) # this is a directory in the Nylon source, leave this
   add_definitions( -DUNICODE=1 )
   link_directories( c:/usr/share/lua523 c:/usr/share/lua523/vendor/luabind/lib ) # link directories must be specified prior to the library; see http://stackoverflow.com/questions/8190103/cmake-library-not-linking
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  execute_process( COMMAND pkg-config --cflags glib-2.0 OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBCFLAGS )
  execute_process( COMMAND pkg-config --libs glib-2.0   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBLDFLAGS )

  SET ( CMAKE_CXX_FLAGS "${GLIBCFLAGS} --std=c++0x" )
  include_directories( /usr/include/lua5.2 ./linux-gtk ) # this is a directory in the Nylon source, leave this
endif()


add_library (syscore SHARED 
  NylonSysCore.cpp
)

#set_target_properties(syscore PROPERTIES COMPILE_FLAGS "-save-temps") 

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries( syscore lua523d luabind09-d )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries( syscore lua5.2 luabind rt "${GLIBLDFLAGS}")
endif()

# For lua extensions, we don't want it to be "libsyscore.so" but just "syscore.so"
# http://stackoverflow.com/questions/1564696/cmake-output-name-for-dynamic-loaded-library
set_target_properties( syscore PROPERTIES PREFIX "")

if ( MSVC )
   # http://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
#    set_target_properties( ${targetname} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${youroutputdirectory} )
    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
endif ( MSVC )


