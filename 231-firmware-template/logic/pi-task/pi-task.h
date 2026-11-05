// pi-task.h — задача вычисления ПИ
#pragma once

#include <stdbool.h>
#include <stdint.h>

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
// обработчик задачи должен уложиться в 0,5 мс, а это около ста членов
#define PI_TERMS_PER_CALL 100

typedef enum
{
    NOT_STARTED,
    RUNNING,
    READY,
} pi_state_t;

bool pi_task_start(uint32_t n);
void pi_task_handle(void);
pi_state_t pi_task_get_result(double *result, uint64_t *spent_time);