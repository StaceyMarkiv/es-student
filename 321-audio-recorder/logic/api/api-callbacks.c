#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "api/api-commands.h"
#include "button-task/button-task.h"
#include "block-task/block-task.h"
#include "firmware.h"
#include "led-task/led-task.h"
#include "pi-task/pi-task.h"
#include "platform/platform.h"
#include "stream-task/stream-task.h"
#include "telemetry-task/telemetry-task.h"

#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"

// период телеметрии по умолчанию
#define TELEMETRY_DEFAULT_PERIOD_MS 100

// период индикатора записи во время потока
#define STREAM_BLINK_PERIOD_MS 250

// окно mic_level по умолчанию и окно mic_bias
#define MIC_DEFAULT_WINDOW_MS 100

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

static void cmd_get_voltage(const command_t *command)
{
    if (!platform_adc_busy())
    {
        // напряжение на GP26 у platform, ответ voltage: <напряжение> V
        printf("voltage: %.3f V\n", platform_voltage_read());
    }
    else
    {
        printf("error: adc busy");
    }
}

static void cmd_get_temp(const command_t *command)
{
    if (!platform_adc_busy())
    {
        // температура кристалла, ответ temp: <температура> C
        printf("temp: %.1f C\n", platform_temp_read());
    }
    else
    {
        printf("error: adc busy");
    }
}

void api_print_telemetry(const telemetry_record_t *record)
{
    // печатает запись телеметрии строкой tm: <время, мкс> <напряжение, В> <температура, °C>
    printf("tm: %llu %.3f %0.1f\n", record->time_us, record->voltage_v, record->temp_c);
}

static void cmd_tm_start(const command_t *command)
{
    if (!platform_adc_busy())
    {
        uint32_t print_period;
        bool isNumber = true;
        if (command->argc == 1)
            print_period = TELEMETRY_DEFAULT_PERIOD_MS;
        else if (command->argc == 2)
            isNumber = parse_u32(command->argv[0], &print_period);
        else
            isNumber = false;

        if (isNumber)
        {
            if (print_period > 0)
            {
                bool telemetry_print_started = telemetry_task_start(print_period);

                if (telemetry_print_started)
                {
                    printf("telemetry: started, period %u ms\n", (unsigned)print_period);
                }
                else
                {
                    printf("error: period_ms must be at least 10\n");
                }
            }
            else
            {
                printf("error: period_ms must be greater than 0\n");
            }
        }
        else
        {
            printf("error: usage tm_start [<period_ms>]\n");
        }
    }
    else
    {
        printf("error: adc busy");
    }
}

static void cmd_tm_stop(const command_t *command)
{
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void cmd_charge_on(const command_t *command)
{
    platform_charge_set(true);
    printf("charge: on\n");
}

static void cmd_charge_off(const command_t *command)
{
    platform_charge_set(false);
    printf("charge: off\n");
}

static void cmd_timer_blink_start(const command_t *command)
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
            bool blink_started = platform_blink_start(period);
            if (!blink_started)
            {
                printf("error: period_ms must be 10..60000\n");
            }
            else
            {
                led_task_set_state(LED_STATE_OFF);
                printf("timer_blink: started, period %u ms\n", (unsigned)period);
            }
        }
        else
        {
            printf("error: period_ms must be greater than 0\n");
        }
    }
    else
    {
        printf("error: usage timer_blink_start <period_ms>\n");
    }
}

static void cmd_timer_blink_stop(const command_t *command)
{
    uint32_t toggles = platform_blink_stop();
    printf("timer_blink: stopped, %u toggles\n", (unsigned)toggles);
}

static void cmd_cap_start(const command_t *command)
{
    uint32_t N;
    uint32_t period;
    bool isNumber_N = true;
    bool isNumber_period = true;

    if (command->argc == 3)
    {
        isNumber_N = parse_u32(command->argv[0], &N);
        isNumber_period = parse_u32(command->argv[1], &period);
    }
    else
    {
        isNumber_N = false;
        isNumber_period = false;
    }

    if (isNumber_N == true && isNumber_period == true)
    {
        if (N == 0 || N > PLATFORM_CAPTURE_MAX || period < 10)
        {
            printf("error: N must be 1..2000, period_us at least 10\n");
        }
        else
        {
            if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || platform_adc_busy())
            {
                printf("error: adc busy\n");
            }
            else
            {
                bool capture_started = platform_capture_start(N, period);
                if (!capture_started)
                    printf("error: adc busy\n");
                else
                    printf("capture: started, %u samples, period %u us\n", (unsigned)N, (unsigned)period);
            }
        }
    }
    else
    {
        // аргументов не два или не числа
        printf("error: usage cap_start <N> <period_us>\n");
    }
}

static void cmd_cap_dump(const command_t *command)
{
    if (!platform_capture_done())
    {
        printf("error: no capture\n");
    }
    else
    {
        platform_capture_t capture;
        platform_capture_get(&capture);

        printf("capture: %u samples, period %u us, interval %u..%u us, handler %.1f %%\n",
               (unsigned)capture.count,
               (unsigned)capture.period_us,
               (unsigned)capture.min_interval_us,
               (unsigned)capture.max_interval_us,
               (float)capture.handler_percent);

        uint16_t counter = 0;
        for (uint32_t i = 0; i < capture.count; i++)
        {
            if (counter == 0)
            {
                printf("cap: ");
            }

            printf("%d ", capture.samples[i]);

            if (counter == 15)
            {
                printf("\n");
                counter = 0;
                continue;
            }
            counter++;
        }
        printf("\n");
    }
}

void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    uint64_t expected_us = (uint64_t)count * 1000000 / PLATFORM_SAMPLE_RATE_HZ;

    // наименьший, наибольший и средний код блока;
    uint16_t min_val = samples[0];
    uint16_t max_val = samples[0];
    for (uint32_t i = 1; i < count; i++)
    {
        if (samples[i] < min_val)
            min_val = samples[i];
        if (samples[i] > max_val)
            max_val = samples[i];
    }

    float mean_val = (float)(max_val + min_val) / 2;

    printf("dma_cap: %u samples in %u us, expected %u us, min %u max %u mean %.1f\n",
           (unsigned)count,
           (unsigned)time_us,
           (unsigned)expected_us,
           (unsigned)min_val,
           (unsigned)max_val,
           mean_val);
}

static void cmd_dma_cap(const command_t *command)
{
    if (!platform_adc_busy())
    {
        uint32_t N;
        bool isNumber = true;
        if (command->argc == 2)
            isNumber = parse_u32(command->argv[0], &N);
        else
            isNumber = false;

        if (isNumber)
        {
            if (N > 0 && N <= PLATFORM_BLOCK_MAX)
            {
                if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || platform_adc_busy())
                {
                    printf("error: adc busy\n");
                }
                else
                {
                    // захват с функцией print_dma_cap, ответ придёт, когда блок будет готов
                    bool block_started = block_task_start(N, print_dma_cap);
                    if (!block_started)
                        printf("error: adc busy\n");
                }
            }
            else
            {
                printf("error: N must be 1..8192\n");
            }
        }
        else
        {
            printf("error: usage dma_cap <N>\n");
        }
    }
    else
    {
        printf("error: adc busy");
    }
}

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes)
{
    // байты заголовка и отсчётов как они лежат в памяти
    platform_write(header, sizeof(*header));
    platform_write(data, bytes);
}

static void cmd_stream_start(const command_t *command)
{
    if (!platform_adc_busy())
    {
        uint32_t frame_bits;
        bool isNumber = true;
        if (command->argc == 1)
            frame_bits = 16;
        else if (command->argc == 2)
            isNumber = parse_u32(command->argv[0], &frame_bits);
        else
            isNumber = false;

        if (isNumber && (frame_bits == 8 || frame_bits == 16))
        {
            if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || platform_adc_busy())
            {
                printf("error: adc busy\n");
            }
            else
            {
                bool stream_started = stream_task_start(frame_bits);
                if (!stream_started)
                {
                    printf("error: adc busy\n");
                }
                else
                {
                    printf("stream: started, 44100 Hz, %u bit\n", (unsigned)frame_bits);

                    platform_blink_start(STREAM_BLINK_PERIOD_MS);
                    led_task_set_state(LED_STATE_OFF);
                }
            }
        }
        else
        {
            printf("error: usage stream_start [8|16]\n");
        }
    }
    else
    {
        printf("error: adc busy\n");
    }
}

static void cmd_stream_stop(const command_t *command)
{
    if (stream_task_get_state() == STREAM_STATE_RUN)
    {
        // если поток идёт — остановить задачу и мигание;
        stream_task_stop();
        platform_blink_stop();

        printf("stream: stopped, %u frames, %u samples, %u overruns\n",
               (unsigned)stream_task_get_frames(),
               (unsigned)stream_task_get_samples(),
               (unsigned)stream_task_get_overruns());
    }
}

static void cmd_generate_tone(const command_t *command)
{
    uint32_t generate_tone;
    bool isNumber = true;

    if (command->argc == 2)
        isNumber = parse_u32(command->argv[0], &generate_tone);
    else
        isNumber = false;

    if (isNumber)
    {
        bool tone_set = platform_tone_set(generate_tone);
        if (!tone_set)
        {
            printf("error: Hz must be 0 or 20..20000\n");
        }
        else
        {
            if (generate_tone == 0)
            {
                printf("tone: off\n");
            }
            else
            {
                printf("tone: %u Hz\n", (unsigned)generate_tone);
            }
        }
    }
    else
    {
        printf("error: usage generate_tone <Hz>\n");
    }
}

static void cmd_hang(const command_t *command)
{
    printf("hang: superloop stopped\n");

    // суперцикл больше не идёт, прерывания приходят как прежде
    while (1)
    {
    }
}

static void cmd_wdt_enable(const command_t *command)
{
    uint32_t timeout_ms;
    bool isNumber = true;

    if (command->argc == 2)
        isNumber = parse_u32(command->argv[0], &timeout_ms);
    else
        isNumber = false;

    if (isNumber)
    {
        bool timeout_enabled = platform_watchdog_enable(timeout_ms);
        if (!timeout_enabled)
        {
            printf("error: timeout_ms must be 1..8388\n");
        }
        else
        {
            printf("wdt: enabled, timeout %u ms\n", (unsigned)timeout_ms);
        }
    }
    else
    {
        printf("error: usage wdt_enable <timeout_ms>\n");
    }
}

static void cmd_reset_reason(const command_t *command)
{
    uint32_t restarts;
    bool is_watchdog_reset = platform_watchdog_caused_reset(&restarts);
    printf("reset: %s, restarts %u\n", is_watchdog_reset ? "watchdog" : "other", (unsigned)restarts);
}

// среднее и отклонение от среднего в кодах АЦП: среднеквадратичное и наибольшее
static void block_stats(const uint16_t *samples, uint32_t count, float *mean, float *rms, float *peak)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }
    int32_t center = (int32_t)(sum / count);

    // в целых числах: в float на 8192 отсчётах ядро считало бы в 24 раза дольше
    uint64_t squares = 0;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t deviation = (int32_t)samples[i] - center;
        uint32_t size = deviation < 0 ? -deviation : deviation;
        squares += (uint64_t)(deviation * deviation);
        if (size > largest)
        {
            largest = size;
        }
    }
    if (*mean)
        *mean = (float)sum / count;
    if (*rms)
        *rms = sqrtf((float)squares / count);
    if (*peak)
        *peak = (float)largest;
}

// среднее блока в вольтах
void print_mic_bias(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    float mean;
    block_stats(samples, count, &mean, NULL, NULL);
    printf("mic: bias %.3f V\n", platform_adc_volts(mean));
}

//  отклонения в милливольтах, окно — count * 1000 / 44100
void print_mic_level(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    float rms, peak;
    block_stats(samples, count, NULL, &rms, &peak);
    printf("mic: level %.1f mV rms, peak %.1f mV, %u ms\n",
           platform_adc_volts(rms) * 1000,
           platform_adc_volts(peak) * 1000,
           (unsigned)(count * 1000 / 44100));
}

static void cmd_mic_bias(const command_t *command)
{
    if (!platform_adc_busy())
    {
        uint32_t count = MIC_DEFAULT_WINDOW_MS * 44100 / 1000;
        if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || platform_adc_busy())
        {
            printf("error: adc busy\n");
        }
        else
        {
            // захват с функцией print_mic_bias, ответ придёт, когда блок будет готов
            bool block_started = block_task_start(count, print_mic_bias);
            if (!block_started)
                printf("error: adc busy\n");
        }
    }
    else
    {
        printf("error: adc busy");
    }
}

static void cmd_mic_level(const command_t *command)
{
    if (!platform_adc_busy())
    {
        uint32_t ms;
        bool isNumber = true;
        if (command->argc == 1)
            ms = MIC_DEFAULT_WINDOW_MS;
        else if (command->argc == 2)
            isNumber = parse_u32(command->argv[0], &ms);
        else
            isNumber = false;

        if (isNumber)
        {
            uint64_t count = ms * 44100 / 1000;

            if (count > 0 && count <= PLATFORM_BLOCK_MAX)
            {
                if (telemetry_task_get_state() == TELEMETRY_STATE_RUN || platform_adc_busy())
                {
                    printf("error: adc busy\n");
                }
                else
                {
                    // захват с функцией print_mic_level, ответ придёт, когда блок будет готов
                    bool block_started = block_task_start(count, print_mic_level);
                    if (!block_started)
                        printf("error: adc busy\n");
                }
            }
            else
            {
                printf("error: ms must be 1..185\n");
            }
        }
        else
        {
            printf("error: usage mic_level [<ms>]\n");
        }
    }
    else
    {
        printf("error: adc busy");
    }
}

// start_block(): запуск захвата теперь нужен трём командам, вынести его из cmd_dma_cap();

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
    {"get_voltage", "voltage on GP26, ADC input 0", cmd_get_voltage},
    {"get_temp", "chip temperature, degrees Celsius", cmd_get_temp},
    {"tm_start", "start reading telemetry", cmd_tm_start},
    {"tm_stop", "stop reading telemetry", cmd_tm_stop},
    {"charge_on", "rc-charge on", cmd_charge_on},
    {"charge_off", "rc-charge off", cmd_charge_off},
    {"timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start},
    {"timer_blink_stop", "stop timer blink, number of toggles", cmd_timer_blink_stop},
    {"cap_start", "start capture", cmd_cap_start},
    {"cap_dump", "get capture dump", cmd_cap_dump},
    {"dma_cap", "dma capture", cmd_dma_cap},
    {"stream_start", "start stream", cmd_stream_start},
    {"stream_stop", "stop stream", cmd_stream_stop},
    {"generate_tone", "generate tone", cmd_generate_tone},
    {"hang", "stop superloop forever", cmd_hang},
    {"wdt_enable", "enabke watchdog timer", cmd_wdt_enable},
    {"reset_reason", "get reset reason", cmd_reset_reason},
    {"mic_bias", "get mic bias", cmd_mic_bias},
    {"mic_level", "get mic level", cmd_mic_level},
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);
