#include "button-task/button-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include <string.h>

typedef enum
{
    RELEASED,
    PRESSING,
    PRESSSED,
    RELEASING,
} button_state_t;

static const uint64_t DEBOUNCE_MS = 20 * 1000;

static button_state_t button_state;
static uint32_t press_count;
static uint64_t last_press;
static button_task_callback_t on_press_func;

void button_task_init(button_task_callback_t on_press)
{
    button_state = RELEASED;
    press_count = 0;
    last_press = 0;
    on_press_func = on_press;
}

void button_task_handle(void)
{
    // читаем уровень: нажатая кнопка замыкает вывод на землю и даёт ноль
    bool button_pressed = !platform_button_read();
    // читаем часы
    uint64_t now_us = systime_us();

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
            // при переходе в «нажата» увеличиваем счётчик и вызываем on_press_func()
            button_state = PRESSSED;
            press_count++;
            if (on_press_func != NULL)
            {
                on_press_func();
            }
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
