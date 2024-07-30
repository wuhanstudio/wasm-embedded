## WASM Embedded

The same WASM code running on different platforms.

| Platform  | Hardware        | Runtime            | GUI         | Language   | Demo                      |
| --------- | --------------- | ------------------ | ----------- | ---------- | ------------------------- |
| Browser   | x64             | v8 engine (Chrome) | canvas      | Javascript | [Click here](./browser)   |
| Linux x64 | x64             | wasm3              | pygame      | Python     | [Click here](./linux)     |
| Arm Linux | aarch64 / armhf | wasm3              | framebuffer | Python     | [Click here](./arm-linux) |
| RTOS      | Cortex-M        | WAMR               | LVGL        | C / C++    | Click here                |
| Arduino   | esp32           | wasm3              | ST7735 TFT  | C / C++    | [Click here](./arduino)   |



![](dino.gif)



![](fire.gif)
