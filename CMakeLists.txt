cmake_minimum_required(VERSION 3.10)
project(MyWasmProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include dir
include_directories("/home/ian/.wasmer/include")
link_directories("/home/ian/.wasmer/lib")

# Add executable
add_executable(host src/host.cpp)
target_link_libraries(host "wasmer")

# build wasm binary: now the global variable has to be exposed here...
add_custom_command(TARGET host PRE_BUILD
  COMMAND ${CMAKE_COMMAND} -E echo "Building a.wasm"  
  COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ ../src/a.cpp -o a.wasm
  # COMMAND /opt/wasi-sdk-25.0-arm64-linux/bin/clang++ -Wl,--export=glob_int ../src/a.cpp -o a.wasm
  COMMAND wasm2wat a.wasm -o a.wat
)

# Add specific files to be cleaned
set_directory_properties(PROPERTIES ADDITIONAL_CLEAN_FILES 
    "a.wasm;a.wat")