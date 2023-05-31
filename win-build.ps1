
del -r -for build-win
mkdir build-win
pushd build-win
cmake "-DLUA_INCLUDE_DIR=c:\opt\lua54\src" -DCMAKE_GENERATOR_PLATFORM=Win32 -G "Visual Studio 17 2022" ..
msbuild nylon.sln /p:Configuration=Release /p:CharacterSet=Unicode
popd
