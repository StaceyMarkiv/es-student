#include "button-task.h"
#include "led-task.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

static const uint BUTTON_PIN = 15;
static const uint DEBOUNCE_MS = 20 * 1000;

static button_state_t button_state;
static uint32_t press_count;
static uint64_t last_press;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = RELEASED;
    press_count = 0;
    last_press = 0;
}

void button_task_handle(void)
{
    // читаем уровень: нажатая кнопка замыкает вывод на землю и даёт ноль
    bool button_pressed = !gpio_get(BUTTON_PIN);
    // читаем часы
    uint64_t now_us = time_us_64();

    // переходы по таблице
    switch (button_state)
    {
    case RELEASED:
        if (button_pressed)
        {
            last_press = now_us;
            button_state = PRESSING;
        }
        break;
    case PRESSING:
        if (!button_pressed)
        {
            button_state = RELEASED;
            break;
        }

        if (now_us - last_press >= DEBOUNCE_MS)
        {
            // при переходе в «нажата» увеличиваем счётчик и вызываем led_task_next_state()
            button_state = PRESSSED;
            press_count++;
            led_task_next_state();
        }
        break;
    case PRESSSED:
        if (!button_pressed)
        {
            last_press = now_us;
            button_state = RELEASING;
        }
        break;
    case RELEASING:
        if (button_pressed)
        {
            button_state = PRESSSED;
            break;
        }

        if (now_us - last_press >= DEBOUNCE_MS)
        {
            button_state = RELEASED;
        }
        break;
    }
}

bool button_task_is_pressed(void)
{
    switch (button_state)
    {
    case RELEASED:
    case PRESSING:
        return false;
    case PRESSSED:
    case RELEASING:
        return true;
    default:
        return false;
    }
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}
