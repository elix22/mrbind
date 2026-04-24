# cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/llvm

cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/llvm -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++
cmake --build build -j4