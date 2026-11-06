#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "api/api-commands.h"
#include "button-task/button-task.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"

// прикидка: за член ряда 4 операции с double, 175 + 110 + 190 + 110 = 585 тактов;
// 1 000 000 членов по 585 тактов при 125 МГц — около 4,7 с
static const uint32_t CALC_PI_TERMS = 1000000;

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

static void print_led_info(led_state_t led_status, uint32_t period)
{
    char *led_status_name;
    switch (led_status)
    {
    case LED_STATE_OFF:
        led_status_name = "off";
        break;
    case LED_STATE_ON:
        led_status_name = "on";
        break;
    case LED_STATE_BLINK:
        led_status_name = "blink";
        break;
    default:
        led_status_name = "";
        break;
    }

    printf("led: %s, period %u ms\n", led_status_name, (unsigned)period);
}

static void cmd_info(const command_t *command)
{
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", FIRMWARE_REPO);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.pico_sdk);
}

static void cmd_uptime(const command_t *command)
{
    printf("uptime: %llu ms\n", systime_ms());
}

static void cmd_pi_start(const command_t *command)
{
    uint32_t members_qty;
    bool isNumber = true;
    if (command->argc == 1)
        members_qty = CALC_PI_TERMS;
    else if (command->argc == 2)
        isNumber = parse_u32(command->argv[0], &members_qty);
    else
        isNumber = false;

    if (isNumber)
    {
        if (members_qty > 0)
        {
            bool pi_calc_started = pi_task_start(members_qty);

            if (pi_calc_started)
            {
                printf("pi: started, %u terms\n", (unsigned)members_qty);
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
        const char *terms_err;
        if (command->argc > 2)
            terms_err = "args > 2";
        else
            terms_err = command->argv[0];

        printf("error: usage pi_start[%s]\n", terms_err);
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
    print_led_info(LED_STATE_ON, received_period);
}

static void cmd_led_disable(const command_t *command)
{
    led_task_set_state(LED_STATE_OFF);

    uint32_t received_period = led_task_get_period_ms();
    print_led_info(LED_STATE_OFF, received_period);
}

static void cmd_led_blink(const command_t *command)
{
    led_task_set_state(LED_STATE_BLINK);

    uint32_t received_period = led_task_get_period_ms();
    print_led_info(LED_STATE_BLINK, received_period);
}

static void cmd_led_period(const command_t *command)
{
    uint32_t period;
    bool isNumber = true;

    if (command->argc == 2)
        isNumber = parse_u32(command->argv[0], &period);
    else
        isNumber = false;

    if (isNumber)
    {
        if (period > 0)
        {
            led_task_set_period_ms(period);
            print_led_info(led_task_get_state(), period);
        }
        else
        {
            printf("error: period_ms must be greater than 0\n");
        }
    }
    else
    {
        printf("error: usage led_period <period_ms>\n");
    }
}

static void cmd_button(const command_t *command)
{
    printf("button: %s, presses %u\n", button_task_is_pressed() ? "pressed" : "released", (unsigned)button_task_get_press_count());
}

static void cmd_profiling(const command_t *command)
{
    if (PROFILING_ENABLED == 0)
    {
        printf("profiling is disabled in this build\n");
        return;
    }

    for (uint32_t id = 0; id < STOPWATCH_QTY; id++)
    {
        profiling_result_t stopwatch;
        bool exists = profiling_get(id, &stopwatch);
        if (exists != false)
        {
            printf("%-11s count %-11u mean %.2f us max %d us\n", stopwatch.name, (unsigned)stopwatch.count, ((double)stopwatch.avg_sum / stopwatch.count), (unsigned)stopwatch.max_us);
        }
        else
        {
            printf("no stopwatch with id %d\n", (unsigned)id);
        }
    }
}

static void cmd_profiling_reset(const command_t *command)
{
    profiling_reset();
    printf("profiling reset\n");
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

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
