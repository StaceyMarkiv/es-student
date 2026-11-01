#include "led-task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Состояние задачи: внутреннее связывание, только для этого файла
static led_state_t led_state;
static uint64_t half_period_us;
static uint64_t last_toggle_us;

static const uint LED_PIN = 16;

// Вспомогательная функция: из других файлов её не вызвать
static void led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

static bool led_get(void)
{
    return gpio_get(LED_PIN);
}

static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();

    if (now_us - *last_us < period_us)
        return false;

    // Следующее срабатывание отсчитывается от прошлого, а не от «сейчас»
    *last_us += period_us;

    // Отстали больше чем на период: не догонять, а начать отсчёт заново
    if (now_us - *last_us >= period_us)
        *last_us = now_us;

    return true;
}

void led_task_init(void)
{
    // настраиваем вывод светодиода, период 1000 мс, состояние «мигает»
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    half_period_us = 500000;
    led_state = LED_STATE_BLINK;
    last_toggle_us = 0;
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
        if (period_elapsed(&last_toggle_us, half_period_us))
        {
            led_set(!led_get());
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
        led_set(false);
        break;
    case LED_STATE_ON:
        led_set(true);
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
