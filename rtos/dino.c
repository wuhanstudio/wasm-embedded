
/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <rtthread.h>
#include <wasm_export.h>

#include <wasm_c_api.h>
#include "dino.wasm.h"

wasm_memory_t* get_export_memory(const wasm_extern_vec_t* exports, size_t i) {
  if (exports->size <= i || !wasm_extern_as_memory(exports->data[i])) {
    printf("> Error accessing memory export %zu!\n", i);
    exit(1);
  }
  return wasm_extern_as_memory(exports->data[i]);
}

float math_random(wasm_exec_env_t exec_env)
{
    float r = (float) rand() / (RAND_MAX + 1.0);
    return r;
}

// Define an array of NativeSymbol for the APIs to be exported.
// Note: the array must be static defined since runtime
//            will keep it after registration
// For the function signature specifications, goto the link:
// https://github.com/bytecodealliance/wasm-micro-runtime/blob/main/doc/export_native_api.md

static NativeSymbol native_symbols[] = {
    {
        "random", // the name of WASM function name
        math_random,   // the native function pointer
        "f()",  // the function prototype signature, avoid to use i32
        NULL        // attachment is NULL
    },
};

#define THREAD_PRIORITY         15
#define THREAD_STACK_SIZE       10*1024
#define THREAD_TIMESLICE        5

static rt_thread_t tid1 = RT_NULL;

static void thread1_entry(void *parameter)
{
    //    static char global_heap_buf[128 * 1024];

        char error_buf[128];

        wasm_module_t module = NULL;
        wasm_module_inst_t module_inst = NULL;
        wasm_exec_env_t exec_env = NULL;
        uint32 stack_size = 8 * 1024, heap_size = 64 * 1024;
        wasm_function_inst_t func = NULL;

        uint64_t wasm_buffer = 0;

        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

    //    init_args.mem_alloc_type = Alloc_With_Pool;
    //    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    //    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

        init_args.mem_alloc_type = Alloc_With_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = os_malloc;
        init_args.mem_alloc_option.allocator.realloc_func = os_realloc;
        init_args.mem_alloc_option.allocator.free_func = os_free;

        // Native symbols need below registration phase
        init_args.n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol);
        init_args.native_module_name = "Math";
        init_args.native_symbols = native_symbols;

        if (!wasm_runtime_full_init(&init_args)) {
            printf("Init runtime environment failed.\n");
            return;
        }
        wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

        // natives registration must be done before loading WASM modules
        if (!wasm_runtime_register_natives("Math",
                                           native_symbols,
                                           sizeof(native_symbols) / sizeof(NativeSymbol))) {
            goto fail;
        }

        module = wasm_runtime_load((uint8 *)dino_wasm, dino_wasm_len, error_buf,
                                   sizeof(error_buf));
        if (!module) {
            printf("Load wasm module failed. error: %s\n", error_buf);
            goto fail;
        }

        // Print out imports
        wasm_importtype_vec_t importtypes = { 0 };
        wasm_module_imports(module, &importtypes);

        for (unsigned i = 0; i < importtypes.num_elems; i++)
        {
            const wasm_name_t *module_name = wasm_importtype_module(importtypes.data[i]);
            const wasm_name_t *field_name = wasm_importtype_name(importtypes.data[i]);
            rt_kprintf("%s, %s", module_name->data, field_name->data);

//            wasm_functype_t *log_type = wasm_functype_new_2_0(wasm_valtype_new_i64(), wasm_valtype_new_i32());
//            wasm_func_t *log_func = wasm_func_new(store, log_type, host_logs);
//            wasm_functype_delete(log_type);
//
//            externs[i] = wasm_func_as_extern(log_func);
        }

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));

        if (!module_inst) {
            printf("Instantiate wasm module failed. error: %s\n", error_buf);
            goto fail;
        }

        // Extract export.
//        printf("Extracting exports...\n");
//        wasm_extern_vec_t exports;
//        wasm_instance_exports(module_inst, &exports);
//        wasm_memory_t* memory = get_export_memory(&exports, 0);
//        rt_kprintf("Memory size: %ld\n", wasm_memory_size(memory));

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        if (!exec_env) {
            printf("Create wasm execution environment failed.\n");
            goto fail;
        }

        if (!(func = wasm_runtime_lookup_function(module_inst, "run"))) {
            printf("The run wasm function is not found.\n");
            goto fail;
        }

        uint32 arguments[1] = {};

        while(true)
        {
            // pass 4 elements for function arguments
            if (!wasm_runtime_call_wasm(exec_env, func, 0, arguments)) {
                printf("call wasm function run failed. %s\n",
                       wasm_runtime_get_exception(module_inst));
                goto fail;
            }

            rt_thread_mdelay(100);
        }
    fail:
        if (exec_env)
            wasm_runtime_destroy_exec_env(exec_env);
        if (module_inst) {
            if (wasm_buffer)
                wasm_runtime_module_free(module_inst, (uint64)wasm_buffer);
            wasm_runtime_deinstantiate(module_inst);
        }
        if (module)
            wasm_runtime_unload(module);

        wasm_runtime_destroy();

        return;
}

int dino(int argc, char *argv_main[])
{
    tid1 = rt_thread_create("t_dino",
                            thread1_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    return 0;
}
MSH_CMD_EXPORT(dino, wamr dino samples);
