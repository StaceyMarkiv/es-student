#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"

#include "api-commands.h"
#include "button-task.h"
#include "device.h"
#include "led-task.h"
#include "pi-task.h"
#include "profiling.h"

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
static const uint CALC_PI_TERMS = 1000000;

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

static void print_led_info(const char *command_name, uint32_t period)
{
    printf("led: %s, period %u ms\n", command_name, (unsigned)period);
}

static void cmd_info(const command_t *command)
{
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", DEVICE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", DEVICE_PROJECT);
    printf("repo: %s\n", DEVICE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.pico_sdk);
}

static void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t members_qty;
    bool isNumber = true;
    if (command->argc == 1)
        members_qty = CALC_PI_TERMS;
    else
        isNumber = parse_u32(command->argv[0], &members_qty);

    if (isNumber)
    {
        if (members_qty > 0)
        {
            bool pi_calc_started = pi_task_start(members_qty);

            if (pi_calc_started)
            {
                printf("pi: started %u terms\n", (unsigned)members_qty);
            }
            else
            {
                printf("error: pi is busy\n");
            }
        }
        else
        {
            printf("error: terms must be greater than 0\n");
        }
    }
    else
    {
        printf("error: usage pi_start[%s]\n", command->argv[0]);
    }
}

static void cmd_pi(const command_t *command)
{
    double pi;
    uint64_t spent_time;
    pi_state_t pi_state = pi_task_get_result(&pi, &spent_time);

    // Ответить значением или сообщить, что расчёт ещё идёт
    if (pi_state == READY)
    {
        printf("pi: %.9f in %llu ms\n", pi, spent_time / 1000);
    }
    else
    {
        printf("pi: %s\n", pi_state == RUNNING ? "running" : "not started");
    }
}

static void cmd_led_enable(const command_t *command)
{
    led_task_set_state(LED_STATE_ON);

    uint32_t received_period = led_task_get_period_ms();
    print_led_info(command->name, received_period);
}

static void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);

    uint32_t received_period = led_task_get_period_ms();
    print_led_info(command->name, received_period);
}

static void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);

    uint32_t received_period = led_task_get_period_ms();
    print_led_info(command->name, received_period);
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period;
    bool isNumber = parse_u32(command->argv[0], &period);

    if (isNumber)
    {
        if (period > 0)
        {
            led_task_set_period_ms(period);
            print_led_info(command->name, period);
        }
        else
        {
            printf("error: led_period must be greater than 0\n");
        }
    }
    else
    {
        printf("error: usage led_period[%s]\n", command->argv[0]);
    }
}

static void cmd_button(const command_t *command)
{
    printf("button: %s, presses %u\n", button_task_is_pressed() ? "pressed" : "released", (unsigned)button_task_get_press_count());
}

static void cmd_profiling(const command_t *command)
{
    for (uint32_t id = 0; id < STOPWATCH_QTY; id++)
    {
        profiling_result_t stopwatch;
        bool exists = profiling_get(id, &stopwatch);
        if (exists != false)
            printf("%-11s count %-11u mean %-5.2f us  max %d us\n", stopwatch.name, (unsigned)stopwatch.count, stopwatch.avg_sum, (unsigned)stopwatch.max_us);
    }
}

static void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
}

const api_command_t api_commands[] = {
    {"info", "device passport", cmd_info},
    {"uptime", "time since reset", cmd_uptime},
    {"pi_start", "calculate pi: pi_start [terms]", cmd_pi_start},
    {"pi", "get pi value", cmd_pi},
    {"led_enable", "enable led", cmd_led_enable},
    {"led_disable", "disable led", cmd_led_disable},
    {"led_blink", "make led blink", cmd_led_blink},
    {"led_period", "set led blink period", cmd_led_period},
    {"button", "show button state and button presses count", cmd_button},
    {"profiling", "show all stopwatches", cmd_profiling},
    {"profiling_reset", "reset all stopwatches", cmd_profiling_reset},
};

const uint api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
