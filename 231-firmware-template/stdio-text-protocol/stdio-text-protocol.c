#include <stdio.h>

#include "pico/stdlib.h"
#include "stdio-text-protocol.h"

static char line[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint line_length;

void stdio_text_protocol_init(void)
{
    line_length = 0;
}

command_t command;
const command_t *stdio_text_protocol_handle(void)
{
    // читаем символ без ожидания; нет символа — возвращаем NULL
    int symbol = getchar_timeout_us(0);
    if (symbol == PICO_ERROR_TIMEOUT)
        return NULL;

    // конец строки: пустую строку пропускаем, иначе переводим строку в терминале,
    if (symbol == '\r' || symbol == '\n')
    {
        if (line_length == 0)
            return NULL;

        putchar('\n');
        line[line_length] = '\0';

        // делим строку на слова и возвращаем команду
        bool new_arg = true;
        command.argc = 0;
        for (uint32_t i = 0; i <= line_length; i++)
        {
            if (line[i] == '\0')
            {
                new_arg = true;
                continue;
            }
            else
            {
                if (new_arg == true)
                {
                    command.argc++;
                    new_arg = false;
                }
            }

            if (command.argc == 1)
            {
                if (command.name == NULL)
                {
                    command.name = &line[i];
                }
            }
            else if (command.argc > 1 && command.argc <= STDIO_TEXT_PROTOCOL_MAX_ARGS + 1)
            {
                if (command.argv[command.argc - 2] == NULL)
                    command.argv[command.argc - 2] = &line[i];
            }
            else
                command.truncated = true;
        }

        line_length = 0;

        if (command.argc > 0)
        {
            return &command;
        }
        else
        {
            return NULL;
        }
    }

    // Backspace (0x08 или 0x7f): убираем последний символ и стираем его на экране "\b \b"
    if (symbol == '\x08' || symbol == '\x7f')
    {
        if (line_length > 0)
        {
            line_length--;

            putchar('\b');
            putchar(' ');
            putchar('\b');
        }

        return NULL;
    }

    if (line_length + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
    {
        // печатный символ: кладём в буфер и повторяем в терминал;
        putchar(symbol);

        if (symbol == ' ')
        {
            line[line_length] = '\0';
        }
        else
        {
            line[line_length] = (char)symbol;
        }

        line_length++;
        return NULL;
    }
    else
    {
        // буфер полон — помечаем строку как переполненную
        command.truncated = true;
        line_length = 0;
        return &command;
    }
}
