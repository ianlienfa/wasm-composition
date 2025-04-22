`/opt/wasi-sdk-25.0-arm64-linux/bin/clang++ a.cpp -o a.wasm`
`wasmer compile a.wasm -o a.wasmu`
`wasmer run a.wasmu`
`wasm-objdump -x a.wasm`
`wasm2wat a.wasm -o a.wat`
`wat2wasm a.wat -o a.wasm`