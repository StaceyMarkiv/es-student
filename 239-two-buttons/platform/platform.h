#pragma once

#include <stdbool.h>

typedef enum
{
    BUTTON_FREQUENCY, // кнопка переключения частоты мигания светодиода
    BUTTON_SWITCH,    // кнопка переключения режима работы светодиода
    BUTTONS_COUNT,    // общее количество кнопок
} platform_button_t;

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);
bool platform_led_get(void);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(platform_button_t button_id);
