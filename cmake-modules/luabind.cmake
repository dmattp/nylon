message(" CMAKE_ARGS ${GLOBAL_THIRDPARTY_LIB_ARGS}")

ExternalProject_Add(luabind
   PREFIX luabind
#   DEPENDS lua
   SOURCE_DIR ${CMAKE_SOURCE_DIR}/luabind-deboostified
   CMAKE_ARGS
      ${GLOBAL_THIRDPARTY_LIB_ARGS}
      -DLUA_LIBRARIES=${LUA_LIBRARIES}
      -DLUA_INCLUDE_DIR=${LUA_INCLUDE_DIR}
      -DCMAKE_INSTALL_PREFIX=${EXT_INSTALL_PATH}/luabind
)

# this makes a nice .dll, but no .lib file (for msvc/win32 builds, anyway).
# So leave as a static library I guess, I don't care to fix luabind's
# cmake right now.
#       -DLUABIND_BUILD_SHARED=1

