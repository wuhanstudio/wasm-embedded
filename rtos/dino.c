#include <rtthread.h>

#include <wasm_export.h>
#include <bh_read_file.h>
#include <bh_getopt.h>

#include <wasm_c_api.h>
#include "dino.wasm.h"

#include <rtdevice.h>
#include "hal_data.h"
#include <lcd_port.h>

#define THREAD_PRIORITY         15
#define THREAD_STACK_SIZE       10 * 1024
#define THREAD_TIMESLICE        5

#define WAMR_STACK_SIZE         8  * 1024
#define WAMR_HEAP_SIZE          64 * 1024

static rt_thread_t tid_wamr = RT_NULL;
static rt_thread_t tid_wasm = RT_NULL;

struct drv_lcd_device
{
    struct rt_device parent;
    struct rt_device_graphic_info lcd_info;

    void *framebuffer;
};

void lcd_clear(struct drv_lcd_device* lcd) {
    /* white */
    for (int i = 0; i < LCD_BUF_SIZE / 2; i++)
    {
        lcd->lcd_info.framebuffer[2 * i] = 0xFF;
        lcd->lcd_info.framebuffer[2 * i + 1] = 0xFF;
    }
    lcd->parent.control(&lcd->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
}

static wasm_func_t* get_export_func(const wasm_extern_vec_t* exports, size_t i) {
    if (exports->size <= i || !wasm_extern_as_func(exports->data[i])) {
      printf("> Error accessing function export %zu!\n", i);
      exit(1);
    }
    return wasm_extern_as_func(exports->data[i]);
}

static wasm_memory_t* get_export_memory(const wasm_extern_vec_t* exports, size_t i) {
    if (exports->size <= i || !wasm_extern_as_memory(exports->data[i])) {
      printf("> Error accessing memory export %zu!\n", i);
      exit(1);
    }
    return wasm_extern_as_memory(exports->data[i]);
}

static float math_random(wasm_exec_env_t exec_env)
{
    float r = (float) rand() / (RAND_MAX + 1.0);
    return r;
}

// Define an array of NativeSymbol for the APIs to be exported.
// Note: the array must be static defined since runtime
//            will keep it after registration
// For the function signature specifications, goto the link:
// https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md

static NativeSymbol native_symbols_math[] = {
    {
        "random",       // the name of WASM function name
        math_random,    // the native function pointer
        "()f",          // the function prototype signature, avoid to use i32
        NULL            // attachment is NULL
    },
};

/*
 * WAMR API Example
 */
static void dino_thread_entry(void *parameter)
{
    // Initialize the LCD
    struct drv_lcd_device *lcd;
    lcd = (struct drv_lcd_device *)rt_device_find("lcd");

    lcd_clear(lcd);

    char error_buf[128];

    wasm_module_t module = NULL;
    wasm_module_inst_t module_inst = NULL;
    wasm_exec_env_t exec_env = NULL;
    wasm_function_inst_t func = NULL;

    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    // static char global_heap_buf[128 * 1024];
    // init_args.mem_alloc_type = Alloc_With_Pool;
    // init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    // init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    init_args.mem_alloc_type = Alloc_With_Allocator;
    init_args.mem_alloc_option.allocator.malloc_func = os_malloc;
    init_args.mem_alloc_option.allocator.realloc_func = os_realloc;
    init_args.mem_alloc_option.allocator.free_func = os_free;

    // Native symbols need below registration phase
    // init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
    // init_args.native_module_name = "Math";
    // init_args.native_symbols = native_symbols;

    if (!wasm_runtime_full_init(&init_args)) {
        printf("Init runtime environment failed.\n");
        return;
    }
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

    // natives registration must be done before loading WASM modules
    if (!wasm_runtime_register_natives("Math",
                                     native_symbols_math,
                                     sizeof(native_symbols_math) / sizeof(NativeSymbol)))
    {
        printf("> Error linking functions!\n");
        return;
    }

    module = wasm_runtime_load((uint8 *)dino_wasm, dino_wasm_len, error_buf,
                               sizeof(error_buf));
    if (!module) {
        printf("Load wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    module_inst = wasm_runtime_instantiate(module, WAMR_STACK_SIZE, WAMR_HEAP_SIZE,
                                           error_buf, sizeof(error_buf));
    if (!module_inst) {
        printf("Instantiate wasm module failed. error: %s\n", error_buf);
        goto fail;
    }

    /* Use app addr 0 to get the start address of linear memory */
     uint8 *mem_start_addr = wasm_runtime_addr_app_to_native(module_inst, 0);

    exec_env = wasm_runtime_create_exec_env(module_inst, WAMR_STACK_SIZE);
    if (!exec_env) {
        printf("Create wasm execution environment failed.\n");
        goto fail;
    }

    if (!(func = wasm_runtime_lookup_function(module_inst, "run"))) {
        printf("The run wasm function is not found.\n");
        goto fail;
    }

    // Call the WASM function
    uint32 i = 0, arguments[1] = {};

    while(true)
    {
        // Call the WASM function
        if (!wasm_runtime_call_wasm(exec_env, func, 0, arguments)) {
            printf("call wasm function run failed. %s\n",
                   wasm_runtime_get_exception(module_inst));
            goto fail;
        }

        // Draw the framebuffer
        for (int h = 0; h < 75; h++)
        {
            for (int w = 0; w < 300; w++)
            {
                // lcd->lcd_info.framebuffer[i] = mem_start_addr[0x5000+i];
                uint8_t r = mem_start_addr[0x5000 + 4 * (h * 300 + w)];       // Red component
                uint8_t g = mem_start_addr[0x5000 + 4 * (h * 300 + w) + 1];   // Green component
                uint8_t b = mem_start_addr[0x5000 + 4 * (h * 300 + w) + 2];   // Blue component

                // Convert to RGB565
                uint16_t r5 = (r >> 3) & 0x1F;   // Take the most significant 5 bits of Red
                uint16_t g6 = (g >> 2) & 0x3F;   // Take the most significant 6 bits of Green
                uint16_t b5 = (b >> 3) & 0x1F;   // Take the most significant 5 bits of Blue

                uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

                // Split the 16-bit value into two 8-bit values
                lcd->lcd_info.framebuffer[2 * ((h+195) * 480 + w)] = rgb565 & 0xFF;
                lcd->lcd_info.framebuffer[2 * ((h+195) * 480 + w) + 1] = (rgb565 >> 8) & 0xFF;;}
        }

        rt_kprintf("frame %ld\n", i++);
    }
fail:
    if (exec_env)
        wasm_runtime_destroy_exec_env(exec_env);
    if (module_inst)
        wasm_runtime_deinstantiate(module_inst);
    if (module)
        wasm_runtime_unload(module);

    wasm_runtime_destroy();

    return;
}

int dino_wamr_api(int argc, char *argv_main[])
{
    tid_wamr = rt_thread_create("t_dino_wamr",
                            dino_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (tid_wamr != RT_NULL)
        rt_thread_startup(tid_wamr);

    return 0;
}
MSH_CMD_EXPORT(dino_wamr_api, dino example using WAMR API);

/*
 * WASM C API Example
 */
static void dino_c_thread_entry(void *parameter)
{
    struct drv_lcd_device *lcd;
    lcd = (struct drv_lcd_device *)rt_device_find("lcd");

    lcd_clear(lcd);

    // Initialize.
    printf("Initializing...\n");
    wasm_engine_t* engine = wasm_engine_new();
    wasm_store_t* store = wasm_store_new(engine);

    // natives registration must be done before loading WASM modules
    if (!wasm_runtime_register_natives("Math",
                                     native_symbols_math,
                                     sizeof(native_symbols_math) / sizeof(NativeSymbol)))
    {
        printf("> Error linking functions!\n");
        return;
    }

    // Compile.
    printf("Compiling module...\n");
    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, dino_wasm_len);
    binary.data = &dino_wasm;
    wasm_module_t* module = wasm_module_new(store, &binary);
    if (!module) {
      printf("> Error compiling module!\n");
      return;
    }

    // Instantiate.
    printf("Instantiating module...\n");
    wasm_extern_vec_t imports = WASM_EMPTY_VEC;
    wasm_instance_t *instance = wasm_instance_new_with_args(
        store, module, &imports, NULL, KILOBYTE(32), 0);
    if (!instance) {
      printf("> Error instantiating module!\n");
      return;
    }

    // Extract export.
    printf("Extracting exports...\n");
    wasm_extern_vec_t exports;
    wasm_instance_exports(instance, &exports);

    // Get the memory
    wasm_memory_t* memory = get_export_memory(&exports, 0);
    wasm_func_t* run_func = get_export_func(&exports, 1);
    printf("Memory size: %d\n", wasm_memory_data_size(memory));

    wasm_val_t r[] = {WASM_INIT_VAL};
    wasm_val_vec_t args_ = {0, NULL, 0, sizeof(wasm_val_t), NULL};
    wasm_val_vec_t results = WASM_ARRAY_VEC(r);

    uint32 i = 0;
    while(true)
    {
        // Call the WASM function
        wasm_trap_t *trap = wasm_func_call(run_func, &args_, &results);
        if (trap) {
            printf("> Error on result\n");
            wasm_trap_delete(trap);
            break;
        }

        // Draw the framebuffer
        for (int h = 0; h < 75; h++)
        {
            for (int w = 0; w < 300; w++)
            {
                // lcd->lcd_info.framebuffer[i] = mem_start_addr[0x5000+i];
                uint8_t r = wasm_memory_data(memory)[0x5000 + 4 * (h * 300 + w)];       // Red component
                uint8_t g = wasm_memory_data(memory)[0x5000 + 4 * (h * 300 + w) + 1];   // Green component
                uint8_t b = wasm_memory_data(memory)[0x5000 + 4 * (h * 300 + w) + 2];   // Blue component

                // Convert to RGB565
                uint16_t r5 = (r >> 3) & 0x1F;   // Take the most significant 5 bits of Red
                uint16_t g6 = (g >> 2) & 0x3F;   // Take the most significant 6 bits of Green
                uint16_t b5 = (b >> 3) & 0x1F;   // Take the most significant 5 bits of Blue

                uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

                // Split the 16-bit value into two 8-bit values
                lcd->lcd_info.framebuffer[2 * ((h+195) * 480 + w)] = rgb565 & 0xFF;
                lcd->lcd_info.framebuffer[2 * ((h+195) * 480 + w) + 1] = (rgb565 >> 8) & 0xFF;;}
        }

        printf("Frame %ld\n", i++);
    }

    wasm_module_delete(module);

    // Shut down.
    printf("Shutting down...\n");
    wasm_store_delete(store);
    wasm_engine_delete(engine);

    // All done.
    printf("Done.\n");
    return ;
}

int dino_wasm_api(int argc, char *argv_main[])
{
    tid_wasm = rt_thread_create("t_dino_wasm",
                            dino_c_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (tid_wasm != RT_NULL)
        rt_thread_startup(tid_wasm);

    return 0;
}
MSH_CMD_EXPORT(dino_wasm_api, dino example using WASM C APU);
