#include "block-task.h"
#include "platform/platform.h"

#include <stddef.h>

static volatile uint32_t block_count;
static volatile block_task_callback_t on_block_func;

bool block_task_start(uint32_t count, block_task_callback_t on_block)
{
    // захват уже идёт
    if (platform_adc_busy())
        return false;

    // запомнить число отсчётов и функцию
    block_count = count;
    on_block_func = on_block;

    // platform не начал захват — false;
    return platform_block_start(block_count);
}

void block_task_handle(void)
{
    // функции нет или блок не готов — выйти;
    if (on_block_func == NULL || !platform_block_done())
        return;

    // забыть функцию, затем вызвать её с отсчётами, числом и временем
    block_task_callback_t on_block_func_tmp = on_block_func;
    on_block_func = NULL;

    on_block_func_tmp(platform_block_samples(), block_count, platform_block_time_us());
}
