import wasm3
import ctypes
import numpy as np

def render(dataLength, circleStructSize):
    # mem = rt.get_memory(result)
    print(dataLength, circleStructSize)
    # array = np.ndarray([20, 8], dtype = int, buffer = ctypes.c_void_p.from_address(result))
    # wasm_circles = rt.find_function("getCircles")
    # circles = wasm_circles(800, 600)

# Initialize the environment
env = wasm3.Environment()
rt  = env.new_runtime(64*1024)

# Load the wasm module
with open("canvas.wasm", "rb") as f:
    mod = env.parse_module((f.read()))
    img_ptr = mod.get_global("circles")
    rt.load(mod)

    mem = rt.get_memory(0)
    # (img_base,) = struct.unpack("<I", mem[img_ptr : img_ptr+4])
    # region = mem[img_base : img_base + (img_w * img_h * 4)]
    # img = pygame.image.frombuffer(region, img_size, "RGBA")

    # mod.link_function("env", "render", "(ii)", render)

# Run the module
wasm_main = rt.find_function("_main")
# wasm_main = rt.find_function("get_circles")

try:
    result = wasm_main()
    print(result)
except Exception as e:
    print(e)
