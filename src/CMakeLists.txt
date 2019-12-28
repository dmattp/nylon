# Create a library called "syscore" which includes the source file "NylonSysCore.cpp".
# This cmake is rather a mess. 

# This puts the output .so into '../nylon', which is okay for running from source tree,
# but i really need a proper install, this is a hack.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/nylon")

##################################################################
# MS/Windows Options
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
   include_directories( c:/usr/share/lua523/include c:/m/home/pkg/boost_1_55_0 )  # Customize for your location of lua and luabind headers
   include_directories( ./win ) # this is a directory in the Nylon source, leave this
   add_definitions( -DUNICODE=1 )
   # that's some hacky stuff
    # link directories must be specified prior to the library; see http://stackoverflow.com/questions/8190103/cmake-library-not-linking
   link_directories( c:/usr/share/lua523 c:/usr/share/lua523/vendor/luabind/lib )

##################################################################
# Linux Options.
#
# Linux uses glib for the main loop so that we can integrate with GTK toolkit,
# so get CFLAGS and LDFLAGS options needed to include glib support.
##################################################################
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  execute_process( COMMAND pkg-config --cflags glib-2.0 OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBCFLAGS )
  execute_process( COMMAND pkg-config --libs glib-2.0   OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE GLIBLDFLAGS )
  SET ( CMAKE_CXX_FLAGS "${GLIBCFLAGS} --std=c++14" )
  include_directories(
     /usr/include/luabind
     /usr/include/lua5.3
     ${CMAKE_SOURCE_DIR}/luabind-deboostified
     ${CMAKE_SOURCE_DIR}/src/linux-gtk ) # this is a directory in the Nylon source, leave this
endif()


##################################################################
# Main library created is syscore.so or syscore.dll; this will
# be loaded as a lua extension package, so we don't want it to be
# "libsyscore.so" but just "syscore.so";
# see  http://stackoverflow.com/questions/1564696/cmake-output-name-for-dynamic-loaded-library
##################################################################
add_library (syscore SHARED NylonSysCore.cpp)
set_target_properties( syscore PROPERTIES PREFIX "")


##################################################################
# For debugging the build?
#set_target_properties(syscore PROPERTIES COMPILE_FLAGS "-save-temps") 

##################################################################
# The windows paths and names are all highly specific to my personal
# lua builds, sorry about that.  At some point I should clean
# this up to use proper finding of installed lua builds.
##################################################################
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries( syscore lua523d luabind09-d )
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  target_link_libraries( syscore lua rt luabind "${GLIBLDFLAGS}")
endif()



# http://stackoverflow.com/questions/7747857/in-cmake-how-do-i-work-around-the-debug-and-release-directories-visual-studio-2
if ( MSVC )
    # I guess I originally did this because I wanted the shared library to end up in ../nylon, but I don't
    # really like that anymore.
    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
    set_target_properties( syscore PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/../nylon )
endif ( MSVC )


INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/nylon-runner.h" DESTINATION include/nylon)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/linux-gtk/sys/platform-defs.h" DESTINATION include/nylon/sys)

INSTALL(TARGETS syscore DESTINATION /usr/lib/lua/5.3/nylon)

