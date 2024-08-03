## WASM Embedded

The same WASM code running on different platforms.

![](dino.gif)

| Platform  | Hardware        | Runtime            | GUI         | Language   | Demo                      |
| --------- | --------------- | ------------------ | ----------- | ---------- | ------------------------- |
| Browser   | x64             | v8 engine (Chrome) | canvas      | Javascript | [Click here](./browser)   |
| Linux x64 | x64             | wasm3              | pygame      | Python     | [Click here](./linux)     |
| Arm Linux | aarch64 / armhf | wasm3              | framebuffer | Python     | [Click here](./arm-linux) |
| RTOS      | Cortex-M        | WAMR               | LVGL        | C / C++    | [Click here](./rtos)      |
| Arduino   | esp32           | wasm3              | ST7735 TFT  | C / C++    | [Click here](./arduino)   |

![](fire.gif)

### Related Projects

- WASM3 Runtime: https://github.com/wasm3/wasm3
- WASM Micro-Runtime (WAMR): https://github.com/bytecodealliance/wasm-micro-runtime
- WASM Bindary Tool (WABT): https://github.com/WebAssembly/wabt
- WASI Enabled Toolchain: https://github.com/WebAssembly/wasi-sdk
- WASM Module Decoder (WASP): https://github.com/WebAssembly/wasp
- WAMR import functions: https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md
- Understand the WAMR stacks: https://bytecodealliance.github.io/wamr.dev/blog/understand-the-wamr-stacks/
- Understand the WAMR heap: https://bytecodealliance.github.io/wamr.dev/blog/understand-the-wamr-heaps/
