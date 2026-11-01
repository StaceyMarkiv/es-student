#include <string.h>

#include "pico/stdlib.h"
#include "led-task.h"
#include "log.h"
#include "device.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"

int main()
{
    stdio_init_all(); // обязательно для работы com-порта
    profiling_init();
    stdio_text_protocol_init();
    led_task_init();

    while (1)
    {
        profiling_iteration();
        led_task_handle();

        // принятая строка становится командой, команда — действием прибора
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}