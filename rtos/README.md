## WASM (RT-Thread RTOS)

This demo uses the (RT-Thread RA6M3 HMI Board).

You may also use pre-compiled firmware in this folder: rtthread.hex, rtthread.elf

```
$ git clone https://github.com/wuhanstudio/wasm-embedded
$ git clone https://github.com/RT-Thread/rt-thread
$ cp wasm-embedded/rtos/rtconfig.h rt-thread/bsp/renesas/ra6m3-hmi-board
$ cp wasm-embedded/rtos/rt-thread/* rt-thread/bsp/renesas/ra6m3-hmi-board/src/
$ cd rt-thread/bsp/renesas/ra6m3-hmi-board
$ pkgs --update
$ scons
```

```
# Check imported functions
$ wasm-objdump -j import -x dino.wasm

# Check exported functions and memory
$ wasm-objdump -j export -x dino.wasm
```
