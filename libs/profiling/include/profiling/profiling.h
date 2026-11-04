#pragma once

#include <stdint.h>

// по умолчанию профилирование включено
#ifndef PROFILING_ENABLED
#define PROFILING_ENABLED 1
#endif

#if PROFILING_ENABLED
void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);
#else
// профилирование выключено: вызовы ничего не делают
#define profiling_start(id) ((void)0)
#define profiling_stop(id) ((void)0)
#endif

// число секундомеров
#define STOPWATCH_QTY 5

typedef struct
{
    char *name;
    uint32_t count;
    uint32_t avg_sum; // сумма замеров одного секундомера переполнится через 71 минуту
    uint32_t max_us;
    uint32_t start_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);

bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
