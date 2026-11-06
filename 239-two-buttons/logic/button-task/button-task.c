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
    button_task_callback_t on_press_short_func;
    button_task_callback_t on_press_long_func;
    bool long_pressed;
} button_info_t;

static const uint64_t DEBOUNCE_MS = 20 * 1000;
static const uint64_t LONG_PRESS_MS = 1000 * 1000;

static button_info_t buttons[BUTTONS_COUNT];

void button_task_init(uint32_t button_id, button_task_callback_t on_press_short, button_task_callback_t on_press_long)
{
    buttons[button_id].button_state = RELEASED;
    buttons[button_id].press_count = 0;
    buttons[button_id].last_press = 0;
    buttons[button_id].on_press_short_func = on_press_short;
    buttons[button_id].on_press_long_func = on_press_long;
    buttons[button_id].long_pressed = false;
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
                buttons[button_id].long_pressed = false;
            }
            break;
        case PRESSING:
            if (!button_pressed)
            {
                buttons[button_id].button_state = RELEASED;
                buttons[button_id].long_pressed = false;
                break;
            }

            if (now_us - buttons[button_id].last_press >= DEBOUNCE_MS)
            {
                // перейти в «нажата», запомнить время нажатия; если у кнопки нет долгого действия, сообщить о нажатии
                buttons[button_id].button_state = PRESSSED;
                if (buttons[button_id].on_press_long_func == NULL && buttons[button_id].on_press_short_func != NULL)
                {
                    buttons[button_id].last_press = now_us;
                    buttons[button_id].press_count++;
                    buttons[button_id].on_press_short_func();
                }
            }
            break;
        case PRESSSED:
            if (!button_pressed)
            {
                buttons[button_id].last_press = now_us;
                buttons[button_id].button_state = RELEASING;
                break;
            }

            if (now_us - buttons[button_id].last_press >= LONG_PRESS_MS)
            {
                // сообщить о долгом нажатии, если о нём ещё не сообщали
                if (buttons[button_id].long_pressed == false && buttons[button_id].on_press_long_func != NULL)
                {
                    buttons[button_id].last_press = now_us;
                    buttons[button_id].long_pressed = true;
                    buttons[button_id].on_press_long_func();
                }
            }
            break;
        case RELEASING:
            if (button_pressed)
            {
                buttons[button_id].button_state = PRESSSED;
                break;
            }

            // перейти в «отпущена»; если у кнопки есть долгое действие, а о долгом нажатии не сообщали, сообщить о коротком
            if (now_us - buttons[button_id].last_press >= DEBOUNCE_MS)
            {
                buttons[button_id].button_state = RELEASED;

                if (buttons[button_id].on_press_long_func != NULL && buttons[button_id].on_press_short_func != NULL && buttons[button_id].long_pressed == false)
                {
                    buttons[button_id].last_press = now_us;
                    buttons[button_id].press_count++;
                    buttons[button_id].on_press_short_func();
                }
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
