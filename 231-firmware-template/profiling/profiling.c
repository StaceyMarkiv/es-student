#include "pico/stdlib.h"
#include "profiling.h"

#include <stdio.h>

profiling_result_t stopwatches[STOPWATCH_QTY];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    stopwatches[id].name = (char *)name;
    stopwatches[id].count = 0;
    stopwatches[id].avg_sum = 0;
    stopwatches[id].max_us = 0;
    stopwatches[id].start_us = 0;
}

void profiling_start(uint32_t id)
{
    stopwatches[id].count++;
    stopwatches[id].start_us = time_us_32();
}

void profiling_stop(uint32_t id)
{
    uint32_t now_us = time_us_32();
    uint32_t iteration_us = now_us - stopwatches[id].start_us;

    if (iteration_us > stopwatches[id].max_us)
        stopwatches[id].max_us = iteration_us;

    double new_avg = (stopwatches[id].avg_sum * (stopwatches[id].count - 1) + iteration_us) / stopwatches[id].count;
    stopwatches[id].avg_sum = new_avg;
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    profiling_result_t stopwatch = stopwatches[id];
    if (stopwatch.name == NULL)
        // секундомер с таким номером не заведён
        return false;

    *result = stopwatch;
    return true;
}

void profiling_reset(void)
{
    for (uint32_t id = 0; id < STOPWATCH_QTY; id++)
    {
        stopwatches[id].count = 0;
        stopwatches[id].avg_sum = 0;
        stopwatches[id].max_us = 0;
        stopwatches[id].start_us = 0;
    }
}
