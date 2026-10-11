#pragma once

#include "pico/types.h"

static uint64_t avg_sum = 0;

void profiling_init(void);
void profiling_iteration(void);
float profiling_avg_us(void);
uint32_t profiling_max_us(void);
void profiling_reset_max(void);
