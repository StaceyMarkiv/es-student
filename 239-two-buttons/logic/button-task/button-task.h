// button-task.h — задача кнопки: отпущена, нажимается, нажата, отпускается
#pragma once

#include <stdbool.h>
#include <stdint.h>

// функция, которую задача вызывает на каждое нажатие
typedef void (*button_task_callback_t)(void);

// что делает нажатие, решается для прибора в целом, а не для кнопки, поэтому используем callback
void button_task_init(uint32_t button_id, button_task_callback_t on_press_short, button_task_callback_t on_press_long);
void button_task_handle(void);

bool button_task_is_pressed(uint32_t button_id);
uint32_t button_task_get_press_count(uint32_t button_id);
