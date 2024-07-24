## WASM (WAT)

```
# Install WABT: https://github.com/WebAssembly/wabt
wat2wasm fire.wat -o fire.wasm
xxd -iC fire.wasm > fire.wasm.h
```
