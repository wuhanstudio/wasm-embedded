## WASM (Emscripten)

```
$ git clone https://github.com/emscripten-core/emsdk
$ cd emsdk
$ source emsdk_env.sh
$ emcc canvas.c -s WASM=1 -o canvas.wasm
```
