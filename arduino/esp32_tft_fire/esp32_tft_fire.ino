/*
   Wasm3 - high performance WebAssembly interpreter written in C.
   Copyright © 2021 Volodymyr Shymanskyy, Steven Massey.
   All rights reserved.
*/

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

#include <wasm3.h>

#include "fire.wasm.h"

#define NATIVE_STACK_SIZE   (32*1024)

#define BUTTON_UP           35
#define BUTTON_DOWN         0

#define DISPLAY_BRIGHTESS   128    // 0..255

#define TFT_CS         26
#define TFT_RST        16 // Or set to -1 and connect to Arduino RESET pin
#define TFT_DC         17

#define TFT_LED  13

// For 1.44" and 1.8" TFT with ST7735 use:
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

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
  //Serial.print("Random: "); Serial.println(r);

  m3ApiReturn(r);
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

  runtime = m3_NewRuntime (env, 80 * 1024, NULL);
  if (!runtime) FATAL("NewRuntime", "failed");

  result = m3_ParseModule (env, &module, fire_16_bit_wasm, sizeof(fire_16_bit_wasm));
  if (result) FATAL("ParseModule", result);

  result = m3_LoadModule (runtime, module);
  if (result) FATAL("LoadModule", result);

  m3_LinkRawFunction (module, "Math",   "random",     "f()",      &Math_random);
 
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

  pinMode(TFT_LED, OUTPUT);
  digitalWrite(TFT_LED, HIGH);

  tft.initR(INITR_144GREENTAB); // Init ST7735R chip, green tab
  tft.setRotation(1);
}


void setup()
{
  Serial.begin(115200);

  Serial.println("\nWasm3 v" M3_VERSION " (" M3_ARCH "), build " __DATE__ " " __TIME__);

  uint32_t tend, tstart;
  TSTART();
  init_device();
  TFINISH("Device init");

  tft.fillScreen(ST77XX_BLACK);

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
    tft.startWrite();
    tft.writePixels((uint16_t*)(mem + 0x4000), 128 * 128, true, true);
    tft.endWrite();

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
