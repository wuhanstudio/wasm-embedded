/*
   Wasm3 - high performance WebAssembly interpreter written in C.
   Copyright © 2021 Volodymyr Shymanskyy, Steven Massey.
   All rights reserved.
*/

#include <Arduino_GFX_Library.h>

#define GFX_BL 13 // default backlight pin, you may replace DF_GFX_BL to actual backlight pin

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
Arduino_DataBus *bus = new Arduino_ESP32SPI(17 /* DC */, 26 /* CS */, 18 /* SCK */, 23 /* MOSI */, GFX_NOT_DEFINED /* MISO */, VSPI /* spi_num */);

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
Arduino_GFX *gfx = new Arduino_ST7735(
  bus, 16 /* RST */, 1 /* rotation */, false /* IPS */,
  128 /* width */, 128 /* height */,
  2 /* col offset 1 */, 3 /* row offset 1 */,
  2 /* col offset 2 */, 1 /* row offset 2 */);

#include <wasm3.h>
#define NATIVE_STACK_SIZE   (32*1024)

#define BUTTON_UP           35
#define BUTTON_DOWN         0

/*
   Dino game by by Ben Smith (binji)
     https://github.com/binji/raw-wasm/tree/master/dino
   To build:
     export PATH=/opt/wasp/build/src/tools:$PATH
     wasp wat2wasm --enable-numeric-values -o dino.wasm dino.wat
     xxd -iC dino.wasm > dino.wasm.h

   Note: In Arduino IDE, select Tools->Optimize->Faster (-O3)
*/
#include "dino.wasm.h"

/*
   Engine start, liftoff!
*/

#define FATAL(func, msg) { Serial.print("Fatal: " func " "); Serial.println(msg); while(1) { delay(100); } }
#define TSTART()         { tstart = micros(); }
#define TFINISH(s)       { tend = micros(); Serial.print(s " in "); Serial.print(tend-tstart); Serial.println(" us"); }

// The Math.random() function returns a floating-point,
// pseudo-random number in the range 0 to less than 1
m3ApiRawFunction(Math_random)
{
  m3ApiReturnType (float)
  float r = (float)random(INT_MAX) / INT_MAX;

  m3ApiReturn(r);
}

// Memcpy is generic, and much faster in native code
m3ApiRawFunction(Dino_memcpy)
{
  m3ApiGetArgMem  (uint8_t *, dst)
  m3ApiGetArgMem  (uint8_t *, src)
  m3ApiGetArgMem  (uint8_t *, dstend)

  do {
    *dst++ = *src++;
  } while (dst < dstend);

  m3ApiSuccess();
}

IM3Environment  env;
IM3Runtime      runtime;
IM3Module       module;
IM3Function     func_run;
uint8_t*        mem;

void load_wasm()
{
  M3Result result = m3Err_none;

  if (!env) {
    env = m3_NewEnvironment ();
    if (!env) FATAL("NewEnvironment", "failed");
  }

  m3_FreeRuntime(runtime);

  runtime = m3_NewRuntime (env, 1024, NULL);
  if (!runtime) FATAL("NewRuntime", "failed");

  result = m3_ParseModule (env, &module, dino_wasm, sizeof(dino_wasm));
  if (result) FATAL("ParseModule", result);

  result = m3_LoadModule (runtime, module);
  if (result) FATAL("LoadModule", result);

  m3_LinkRawFunction (module, "Math",   "random",     "f()",      &Math_random);
  m3_LinkRawFunction (module, "Dino",   "memcpy",     "v(iii)",   &Dino_memcpy);

  mem = m3_GetMemory (runtime, NULL, 0);
  if (!mem) FATAL("GetMemory", "failed");

  result = m3_FindFunction (&func_run, runtime, "run");
  if (result) FATAL("FindFunction", result);
}

void init_device()
{
  pinMode(BUTTON_UP,   INPUT);
  pinMode(BUTTON_DOWN, INPUT);

  // Try to randomize seed
  randomSeed((analogRead(A5) << 16) + analogRead(A4));
  Serial.print("Random: 0x"); Serial.println(random(INT_MAX), HEX);
}

void setup()
{
  Serial.begin(115200);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  // Init Display
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  gfx->fillScreen(WHITE);

#ifdef GFX_BL
  pinMode(GFX_BL, OUTPUT);
  digitalWrite(GFX_BL, HIGH);
#endif

  Serial.println("\nWasm3 v" M3_VERSION " (" M3_ARCH "), build " __DATE__ " " __TIME__);

  uint32_t tend, tstart;
  TSTART();
  init_device();
  TFINISH("Device init");

  TSTART();
  load_wasm();
  TFINISH("Wasm3 init");

  Serial.println("Running WebAssembly...");

  xTaskCreate(&wasm_task, "wasm3", NATIVE_STACK_SIZE, NULL, 5, NULL);
}

void wasm_task(void*)
{
  M3Result result;
  uint64_t last_fps_print = 0;

  while (true) {
    const uint64_t framestart = micros();

    // Process inputs
    uint32_t* input = (uint32_t*)(mem + 0x0000);
    *input = 0;
    if (LOW == digitalRead(BUTTON_UP)) {
      *input |= 0x1;
    }
    if (LOW == digitalRead(BUTTON_DOWN)) {
      *input |= 0x2;
    }

    // Render frame
    result = m3_CallV (func_run);
    if (result) break;

    // Output to display (Big Endian)
    gfx->draw16bitRGBBitmap(0, 127 - 75, (uint16_t*)(mem + 0x5000), 128, 75);

    const uint64_t frametime = micros() - framestart;
    const uint32_t target_frametime = 1000000 / 50;
    if (target_frametime > frametime) {
      delay((target_frametime - frametime) / 1000);
    }
    if (framestart - last_fps_print > 1000000) {
      Serial.print("FPS: "); Serial.println((uint32_t)(1000000 / frametime));
      last_fps_print = framestart;
    }
  }

  if (result != m3Err_none) {
    M3ErrorInfo info;
    m3_GetErrorInfo (runtime, &info);
    Serial.print("Error: ");
    Serial.print(result);
    Serial.print(" (");
    Serial.print(info.message);
    Serial.println(")");
    if (info.file && strlen(info.file) && info.line) {
      Serial.print("At ");
      Serial.print(info.file);
      Serial.print(":");
      Serial.println(info.line);
    }
  }
}

void loop()
{
  delay(100);
}
