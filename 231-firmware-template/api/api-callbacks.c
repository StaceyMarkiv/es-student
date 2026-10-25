#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "api-commands.h"
#include "device.h"
#include "profiling.h"

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
static const uint CALC_PI_TERMS = 1000000;
static volatile double pi_result;

// Число без знака из десятичной записи. false — в записи не только цифры
// или число не помещается в uint32_t.
static bool parse_u32(const char *text, uint32_t *value)
{
    // strtoul приняла бы и знак: из "-1" получилось бы 4294967295
    if (!isdigit((unsigned char)text[0]))
    {
        return false;
    }

    char *end;
    errno = 0;
    unsigned long number = strtoul(text, &end, 10);

    // после числа остались символы или число не поместилось
    if (*end != '\0' || errno == ERANGE || number > UINT32_MAX)
    {
        return false;
    }

    *value = (uint32_t)number;
    return true;
}

static double calc_pi(uint terms)
{
    double sum = 0.0;
    double sign = 1.0;

    for (uint k = 0; k < terms; k++)
    {
        // Прибавить очередной член ряда: ±1 / (2k + 1)
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }

    // Сумма ряда равна π/4
    return 4.0 * sum;
}

static void cmd_info(const command_t *command)
{
    device_info();
}

static void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void cmd_calc_pi(const command_t *command)
{
    uint32_t members_qty;
    bool isNumber = parse_u32(command->argv[0], &members_qty);

    if (isNumber)
    {
        if (members_qty > 0)
        {
            uint64_t start_us = time_us_64();
            pi_result = calc_pi(members_qty);
            uint64_t spent_us = time_us_64() - start_us;

            printf("pi: %.9f\n", pi_result);
            printf("time: %llu ms\n", spent_us / 1000);
        }
        else
        {
            printf("error: terms must be gerater than 0\n");
        }
    }
    else
    {
        printf("error: usage pi_start[%s]\n", command->argv[0]);
    }
}

static void cmd_main_time_exec(const command_t *command)
{
    printf("iteration avg %.2f us, max %u us\n", profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *command)
{
    profiling_reset_max();
    printf("%s", "max reset\n");
}

const api_command_t api_commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"calc_pi", "calculate pi: calc_pi [terms]", cmd_calc_pi},
    {"main_time_exec", "superloop iteration: average and maximum", cmd_main_time_exec},
    {"main_time_reset", "reset maximum iteration", cmd_main_time_reset},
};

const uint api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
