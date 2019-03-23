del -r -for build
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -DCMAKE_TOOLCHAIN_FILE=../toolchain/v141xp.cmake ..
cmake --build . --target ALL_BUILD --config Release
