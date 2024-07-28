## WASM Embedded

The same WASM code running on different platforms.

| Platform             | Hardware        | Runtime                                                      | GUI         | Language   |
| -------------------- | --------------- | ------------------------------------------------------------ | ----------- | ---------- |
| [Browser](./browser) | x64             | [v8 engine](https://v8.dev/) (Chrome)                        | canvas      | Javascript |
| [Linux x64](./linux) | x64             | WAMR & wasm3                                                 | pygame      | Python     |
| Arm Linux            | aarch64 / armhf | WAMR & wasm3                                                 | framebuffer | Python     |
| RTOS                 | Cortex-M        | [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) | LVGL        | C / C++    |
| [Arduino](./arduino) | esp32           | [wasm3](https://github.com/wasm3/wasm3)                      | ST7735 TFT  | C / C++    |



![](demo.gif)
