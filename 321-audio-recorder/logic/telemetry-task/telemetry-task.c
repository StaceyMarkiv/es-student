#include "telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

#include <string.h> //нужно для memset

static telemetry_state_t telemetry_state;
static uint32_t work_period;
static uint64_t last_record_us;

static telemetry_record_t telemetry_record;
static telemetry_task_callback_t on_record_func;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    on_record_func = on_record;
    telemetry_state = TELEMETRY_STATE_IDLE;
    work_period = 0;
    last_record_us = 0;
}

void telemetry_task_handle(void)
{
    switch (telemetry_state)
    {
    case TELEMETRY_STATE_RUN:
        if (systime_period_elapsed(&last_record_us, work_period * 1000))
        {
            memset(&telemetry_record, 0, sizeof(telemetry_record)); // обнуление всех полей структуры

            telemetry_record.time_us = systime_us();
            telemetry_record.voltage_v = platform_voltage_read();
            telemetry_record.temp_c = platform_temp_read();

            if (on_record_func != NULL)
            {
                on_record_func(&telemetry_record);
            }
        }
        break;
    case TELEMETRY_STATE_IDLE:
    default:
        break;
    }
}

bool telemetry_task_start(uint32_t period_ms)
{
    // период меньше 10 мс не принять,
    if (period_ms < 10)
    {
        return false;
    }

    telemetry_state = TELEMETRY_STATE_RUN;
    work_period = period_ms;
    last_record_us = systime_us();

    return true;
}

void telemetry_task_stop(void)
{
    telemetry_state = TELEMETRY_STATE_IDLE;
    memset(&telemetry_record, 0, sizeof(telemetry_record)); // обнуление всех полей структуры
}

telemetry_state_t telemetry_task_get_state(void)
{
    return telemetry_state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return work_period;
}
