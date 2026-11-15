#include "pi-task/pi-task.h"
#include "systime/systime.h"

// Состояние расчёта: должно пережить возврат из обработчика
static pi_state_t pi_state = NOT_STARTED;

static uint32_t pi_k;
static uint32_t pi_n;
static double pi_sum;
static double pi_sign;
static double pi_result = 0;

static uint64_t start_us = 0;
static uint64_t spent_us = 0;

bool pi_task_start(uint32_t n)
{
    if (pi_state == RUNNING)
    {
        return false;
    }

    pi_n = n;
    pi_k = 0;
    pi_sum = 0.0;
    pi_sign = 1.0;

    pi_state = RUNNING;
    start_us = systime_us();

    return true;
}

void pi_task_handle(void)
{
    if (pi_state != RUNNING)
    {
        return;
    }

    // Очередная порция членов ряда, но не дальше последнего
    int end = pi_k + PI_TERMS_PER_CALL;
    if (end > pi_n)
    {
        end = pi_n;
    }
    for (; pi_k < end; pi_k++)
    {
        pi_sum += pi_sign / (2.0 * pi_k + 1.0);
        pi_sign = -pi_sign;
    }

    if (pi_k == pi_n)
    {
        pi_result = 4.0 * pi_sum;
        pi_state = READY;

        spent_us = systime_us() - start_us;
    }
}

// Готов ли результат: если да, записать его по указателю
pi_state_t pi_task_get_result(double *result, uint64_t *spent_time)
{
    if (pi_state == READY)
    {
        *result = pi_result;
        *spent_time = spent_us;
    }

    return pi_state;
}
