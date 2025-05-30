cmake_minimum_required(VERSION 3.10)
project(MyWasmProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include dir
# include_directories("/home/ian/.wasmer/include")
# link_directories("/home/ian/.wasmer/lib")
include_directories("/home/ian/wasmer/package/include") # self-compiled version
link_directories("/home/ian/wasmer/package/lib") # self-compiled version

# Add executable
add_executable(host src/host.cpp)
target_link_libraries(host "wasmer")

add_executable(export_import src/export_import.cpp)
target_link_libraries(export_import "wasmer")


# build wasm binary: now the global variable has to be exposed here...
add_custom_command(TARGET host PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Building a.wasm"  
  # COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ ../src/a.cpp -o a.wasm
  # COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ -Wl,--allow-undefined ../src/a.cpp -o a.wasm
  # COMMAND clang++ --target=wasm32 -Wl,--export-all -Wl,--allow-undefined -o a.wasm ../src/a.cpp
  COMMAND em++ -std=c++20 -O2 -sSTANDALONE_WASM -Wl,--allow-undefined -Wl,--no-entry -o a.wasm ../src/a.cpp
  COMMAND wasm2wat a.wasm -o a.wat
)

# build wasm binary: now the global variable has to be exposed here...
add_custom_command(TARGET host PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Building b.wasm"  
  # COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ ../src/a.cpp -o a.wasm
  # COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ -Wl,--allow-undefined ../src/b.cpp -o b.wasm
  COMMAND em++ -std=c++20 -O2 -sSTANDALONE_WASM -Wl,--allow-undefined -Wl,--no-entry -o b.wasm ../src/b.cpp
  COMMAND wasm2wat b.wasm -o b.wat
)

# Add specific files to be cleaned
set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES 
    "a.wasm;a.wat;b.wasm;b.wat")