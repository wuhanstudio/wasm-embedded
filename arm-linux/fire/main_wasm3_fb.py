#!/usr/bin/env python3                           
                           
import wasm3
import os, random

import numpy as np
from PIL import Image
 
print("WebAssembly demo file provided by Ben Smith (binji)")
print("Sources: https://github.com/binji/raw-wasm")
 
scriptpath = os.path.dirname(os.path.realpath(__file__))
wasm_fn = os.path.join(scriptpath, "fire.wasm")
 
# Prepare Wasm3 engine
 
env = wasm3.Environment()
rt = env.new_runtime(1024)
with open(wasm_fn, "rb") as f:
    mod = env.parse_module(f.read())
    rt.load(mod)
    mod.link_function("", "rand", random.random)
 
wasm_run = rt.find_function("run")
mem = rt.get_memory(0)
 
# Map memory region to an RGBA image
img_base = 53760
img_size = (320, 168)
(img_w, img_h) = img_size
region = mem[img_base : img_base + (img_w * img_h * 4)]
 
# Map the screen as numpy array
h, w, c = 480,  800, 4
fb = np.memmap('/dev/fb0', dtype='uint8',mode='w+', shape=(h,w,c))

while True:
    # Render next frame
    wasm_run()
 
    # Resize the image
    img = Image.frombuffer("RGBA", img_size, region, 'raw', "RGBA", 0, 1)
    img = img.resize((800, 480))

    # Write the image to framebuffer
    fb[:, :, :] = np.array(img)[..., [2,1,0,3]]
