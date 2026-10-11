#pragma once

#include "pico/types.h"

// итерация около 2 мкс, за секунду их около 500 000;
// ближайшая степень двойки 2^19 = 524 288, среднее помнит около секунды
#define AVG_SHIFT 19
// замер в 1/256 долях микросекунды, чтобы среднее не теряло дробную часть
#define FRACTION_SHIFT 8

static uint64_t avg_sum = 0;

void profiling_init(void);
void profiling_iteration(void);
float profiling_avg_us(void);
uint32_t profiling_max_us(void);
void profiling_reset_max(void);
