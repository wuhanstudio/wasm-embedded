
/*
 * Copyright (C) 2019 Intel Corporation.  All rights reserved.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#include <rtthread.h>
#include <wasm_export.h>

#include "fib.wasm.h"

#define THREAD_PRIORITY         15
#define THREAD_STACK_SIZE       10*1024
#define THREAD_TIMESLICE        5

static rt_thread_t tid1 = RT_NULL;

static void thread1_entry(void *parameter)
{
        char error_buf[128];

        wasm_module_t module = NULL;
        wasm_module_inst_t module_inst = NULL;
        wasm_exec_env_t exec_env = NULL;
        uint32 stack_size = 8 * 1024, heap_size = 64 * 1024;
        wasm_function_inst_t func = NULL;

        uint64_t wasm_buffer = 0;

        RuntimeInitArgs init_args;
        memset(&init_args, 0, sizeof(RuntimeInitArgs));

        init_args.mem_alloc_type = Alloc_With_Allocator;
        init_args.mem_alloc_option.allocator.malloc_func = os_malloc;
        init_args.mem_alloc_option.allocator.realloc_func = os_realloc;
        init_args.mem_alloc_option.allocator.free_func = os_free;

        if (!wasm_runtime_full_init(&init_args)) {
            printf("Init runtime environment failed.\n");
            return;
        }
        wasm_runtime_set_log_level(WASM_LOG_LEVEL_VERBOSE);

        module = wasm_runtime_load((uint8 *)fib32_wasm, fib32_wasm_len, error_buf,
                                   sizeof(error_buf));
        if (!module) {
            printf("Load wasm module failed. error: %s\n", error_buf);
            goto fail;
        }

        module_inst = wasm_runtime_instantiate(module, stack_size, heap_size,
                                               error_buf, sizeof(error_buf));

        if (!module_inst) {
            printf("Instantiate wasm module failed. error: %s\n", error_buf);
            goto fail;
        }

        exec_env = wasm_runtime_create_exec_env(module_inst, stack_size);
        if (!exec_env) {
            printf("Create wasm execution environment failed.\n");
            goto fail;
        }

        if (!(func = wasm_runtime_lookup_function(module_inst, "fib"))) {
            printf("The generate_float wasm function is not found.\n");
            goto fail;
        }

        wasm_val_t results[1] = { { .kind = WASM_I32, .of.i32 = 0 } };
        wasm_val_t arguments[1] = {
            { .kind = WASM_I32, .of.i32 = 10 },
        };

        // pass 4 elements for function arguments
        if (!wasm_runtime_call_wasm_a(exec_env, func, 1, results, 1, arguments)) {
            printf("call wasm function fib failed. %s\n",
                   wasm_runtime_get_exception(module_inst));
            goto fail;
        }

        // Print result.
        printf("Printing result...\n");
        printf("> %ld\n", results[0].of.i32);

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

int fib(int argc, char *argv_main[])
{
    tid1 = rt_thread_create("t_fib",
                            thread1_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (tid1 != RT_NULL)
        rt_thread_startup(tid1);

    return 0;
}
MSH_CMD_EXPORT(fib, wamr fib samples);
