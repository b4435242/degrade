rm -r build
mkdir build
cd build
cmake .. -DCMAKE_CXX_COMPILER=/usr/bin/clang++
cmake --build .
