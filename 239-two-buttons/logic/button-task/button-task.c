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

typedef struct
{
    button_state_t button_state;
    uint32_t press_count;
    uint64_t last_press;
    button_task_callback_t on_press_func;
} button_info_t;

static const uint64_t DEBOUNCE_MS = 20 * 1000;

static button_info_t buttons[BUTTONS_COUNT];

void button_task_init(uint32_t button_id, button_task_callback_t on_press)
{
    buttons[button_id].button_state = RELEASED;
    buttons[button_id].press_count = 0;
    buttons[button_id].last_press = 0;
    buttons[button_id].on_press_func = on_press;
}

void button_task_handle(void)
{
    for (uint32_t button_id = 0; button_id < BUTTONS_COUNT; button_id++)
    {
        // читаем уровень: нажатая кнопка замыкает вывод на землю и даёт ноль
        bool button_pressed = !platform_button_read(button_id);
        // читаем часы
        uint64_t now_us = systime_us();

        // переходы по таблице
        switch (buttons[button_id].button_state)
        {
        case RELEASED:
            if (button_pressed)
            {
                buttons[button_id].last_press = now_us;
                buttons[button_id].button_state = PRESSING;
            }
            break;
        case PRESSING:
            if (!button_pressed)
            {
                buttons[button_id].button_state = RELEASED;
                break;
            }

            if (now_us - buttons[button_id].last_press >= DEBOUNCE_MS)
            {
                // при переходе в «нажата» увеличиваем счётчик и вызываем on_press_func()
                buttons[button_id].button_state = PRESSSED;
                buttons[button_id].press_count++;
                if (buttons[button_id].on_press_func != NULL)
                {
                    buttons[button_id].on_press_func();
                }
            }
            break;
        case PRESSSED:
            if (!button_pressed)
            {
                buttons[button_id].last_press = now_us;
                buttons[button_id].button_state = RELEASING;
            }
            break;
        case RELEASING:
            if (button_pressed)
            {
                buttons[button_id].button_state = PRESSSED;
                break;
            }

            if (now_us - buttons[button_id].last_press >= DEBOUNCE_MS)
            {
                buttons[button_id].button_state = RELEASED;
            }
            break;
        }
    }
}

bool button_task_is_pressed(uint32_t button_id)
{
    switch (buttons[button_id].button_state)
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

uint32_t button_task_get_press_count(uint32_t button_id)
{
    return buttons[button_id].press_count;
}
