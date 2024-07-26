## WASM (WAT)

```
# Install WASP: https://github.com/WebAssembly/wasp
wasp wat2wasm dino.wat --enable-numeric-values -o dino.wasm
xxd -iC dino.wasm > dino.wasm.h
```
