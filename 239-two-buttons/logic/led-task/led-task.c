#include "led-task/led-task.h"
#include "platform/platform.h"
#include "systime/systime.h"

typedef enum
{
    PERIOD_250,
    PERIOD_500,
    PERIOD_1000,
} set_period_t;

// Состояние задачи: внутреннее связывание, только для этого файла
static led_state_t led_state;
static uint64_t half_period_us;
static uint64_t last_toggle_us;
static set_period_t set_period;

void led_task_init(void)
{
    // период 1000 мс, состояние «мигает»
    half_period_us = 500000;
    led_state = LED_STATE_BLINK;
    last_toggle_us = 0;
    set_period = PERIOD_1000;
}

void led_task_handle(void)
{
    switch (led_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        // «выключен» и «горит» — ничего не делаем, светодиод установлен при входе
        break;
    case LED_STATE_BLINK:
        // «мигает» — каждые полпериода переключаем светодиод
        if (systime_period_elapsed(&last_toggle_us, half_period_us))
        {
            platform_led_set(!platform_led_get());
        }
        break;
    }
}

void led_task_set_state(led_state_t state)
{
    // запоминаем состояние и выполняем действие при входе:
    // «выключен» — гасим, «горит» — зажигаем, «мигает» — первое переключение сразу

    led_state = state;

    switch (led_state)
    {
    case LED_STATE_OFF:
        platform_led_set(false);
        break;
    case LED_STATE_ON:
        platform_led_set(true);
        break;
    case LED_STATE_BLINK:
        last_toggle_us = 0;
        break;
    }
}

led_state_t led_task_get_state(void)
{
    return led_state;
}

bool led_task_set_period_ms(uint32_t period_ms)
{
    // С нулевым полупериодом светодиод переключался бы
    // на каждой итерации: такой период не принимается
    if (period_ms == 0)
    {
        return false;
    }

    half_period_us = (uint64_t)period_ms * 1000 / 2;
    return true;
}

uint32_t led_task_get_period_ms(void)
{
    uint64_t period_ms = half_period_us * 2 / 1000;
    return (uint32_t)period_ms;
}

void led_task_next_state(void)
{
    led_state_t next_state;

    switch (led_task_get_state())
    {
    case LED_STATE_OFF:
        next_state = LED_STATE_ON;
        break;
    case LED_STATE_ON:
        next_state = LED_STATE_BLINK;
        break;
    case LED_STATE_BLINK:
        next_state = LED_STATE_OFF;
        break;
    default:
        next_state = LED_STATE_OFF;
        break;
    }

    led_task_set_state(next_state);
}

void led_task_next_period(void)
{
    uint32_t next_period_val;

    switch (set_period)
    {
    case PERIOD_250:
        set_period = PERIOD_500;
        next_period_val = 500;
        break;
    case PERIOD_500:
        set_period = PERIOD_1000;
        next_period_val = 1000;
        break;
    case PERIOD_1000:
        set_period = PERIOD_250;
        next_period_val = 250;
        break;
    default:
        set_period = PERIOD_1000;
        next_period_val = 1000;
        break;
    }

    led_task_set_period_ms(next_period_val);
}

void led_task_blink(void)
{
    led_task_set_state(LED_STATE_BLINK);
}
