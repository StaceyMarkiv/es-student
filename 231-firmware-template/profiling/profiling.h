#pragma once

#include <stdint.h>
// #include "pico/types.h"

// число секундомеров
#define STOPWATCH_QTY 5

typedef struct
{
    char *name;
    uint32_t count;
    double avg_sum; // сумма замеров одного секундомера переполнится через 71 минуту
    uint32_t max_us;
    uint32_t start_us;
} profiling_result_t;

void profiling_stopwatch_init(uint32_t id, const char *name);

void profiling_start(uint32_t id);
void profiling_stop(uint32_t id);

bool profiling_get(uint32_t id, profiling_result_t *result);
void profiling_reset(void);
