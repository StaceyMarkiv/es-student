#include "platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

static const uint LED_PIN = 16;
static const uint BUTTON_PIN = 15;

void platform_init(void)
{
    // обязательно для работы com-порта
    stdio_init_all();

    // настраиваем вывод светодиода
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // настраиваем кнопку
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    adc_init();
    adc_gpio_init(ADC_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_led_get(void)
{
    return gpio_get(LED_PIN);
}

bool platform_button_read(void)
{
    return gpio_get(BUTTON_PIN);
}

float platform_voltage_read(void)
{
    // функция, которая снимает один отсчёт и переводит его в вольты
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}
