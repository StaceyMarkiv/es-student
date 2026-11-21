#pragma once

#include <stdbool.h>
#include <stdint.h>

// максимальное количество отсчётов GP26 для захвата по будильнику
#define PLATFORM_CAPTURE_MAX 2000
// частота отсчётов АЦП в блоке и потоке
#define PLATFORM_SAMPLE_RATE_HZ 44100
// максимальный размер блока отсчётов GP26 через DMA
#define PLATFORM_BLOCK_MAX 8192

// поток: два буфера по PLATFORM_STREAM_BYTES байт по очереди
#define PLATFORM_STREAM_BYTES 1024

// запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

void platform_led_set(bool on);
bool platform_led_get(void);

// нажата ли кнопка сейчас, без подавления дребезга
bool platform_button_read(void);

// напряжение на входе АЦП, выводе GP26, в вольтах
float platform_voltage_read(void);

// температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void);

void platform_charge_set(bool on);

// индикатор: светодиод мигает по будильнику, независимо от суперцикла;
// false — период меньше 10 мс или больше 60 000 мс
bool platform_blink_start(uint32_t period_ms);

// гасит светодиод и возвращает число переключений с запуска
uint32_t platform_blink_stop(void);

// занят ли АЦП захватом
bool platform_adc_busy(void);

// захват count отсчётов GP26 по будильнику с периодом period_us;
// false — АЦП занят или count больше PLATFORM_CAPTURE_MAX
bool platform_capture_start(uint32_t count, uint32_t period_us);
bool platform_capture_done(void);

// итог захвата по будильнику: отсчёты, разброс интервалов
// и доля времени ядра внутри обработчика, в процентах
typedef struct
{
    const uint16_t *samples;
    uint32_t count;
    uint32_t period_us;
    uint32_t min_interval_us;
    uint32_t max_interval_us;
    float handler_percent;
} platform_capture_t;

void platform_capture_get(platform_capture_t *capture);

// блок count отсчётов GP26 через DMA, о конце сообщает прерывание DMA;
// false — АЦП занят или count больше PLATFORM_BLOCK_MAX
bool platform_block_start(uint32_t count);
bool platform_block_done(void);
const uint16_t *platform_block_samples(void);

// время захвата блока, от запуска АЦП до последнего отсчёта
uint32_t platform_block_time_us(void);

// false — АЦП занят
bool platform_stream_start(uint8_t bits);
void platform_stream_stop(void);

// следующий заполненный буфер по порядку или NULL, если он ещё не готов;
// samples — число отсчётов в буфере
const uint8_t *platform_stream_next(uint32_t *samples);

// буфер отправлен, его можно заполнять снова
void platform_stream_release(void);

// сколько раз DMA застал буфер неотправленным
uint32_t platform_stream_overruns(void);

// отправляет байты в порт как есть, без замены \n на \r\n
void platform_write(const void *data, uint32_t bytes);

// тон на GP20: меандр частоты freq_hz от модуля ШИМ, ядро в нём не участвует;
// 0 — тон выключен, на выводе снова ноль; false — частота вне 20..20000 Гц
bool platform_tone_set(uint32_t freq_hz);

// сторожевой таймер; false — таймаут больше 8388 мс или равен нулю
bool platform_watchdog_enable(uint32_t timeout_ms);
void platform_watchdog_feed(void);

// был ли последний сброс по сторожевому таймеру и сколько таких сбросов подряд
bool platform_watchdog_caused_reset(uint32_t *restarts);
