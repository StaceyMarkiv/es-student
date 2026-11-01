// button-task.h — задача кнопки: отпущена, нажимается, нажата, отпускается
#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    RELEASED,
    PRESSING,
    PRESSSED,
    RELEASING,
} button_state_t;

void button_task_init(void);
void button_task_handle(void);

bool button_task_is_pressed(void);
uint32_t button_task_get_press_count(void);
