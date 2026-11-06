#include "platform.h"

#include "pico/stdlib.h"
#include "hardware/gpio.h"

static const uint LED_PIN = 16;

static uint platform_buttons[BUTTONS_COUNT];

static void platform_button_init(platform_button_t button_pin)
{
    gpio_init(button_pin);
    gpio_set_dir(button_pin, GPIO_IN);
    gpio_pull_up(button_pin);
}

void platform_init(void)
{
    // обязательно для работы com-порта
    stdio_init_all();

    // настраиваем вывод светодиода
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    // настраиваем кнопку переключения режима работы светодиода
    const uint BUTTON_PIN_SWITCH = 15;
    platform_button_init(BUTTON_PIN_SWITCH);
    platform_buttons[BUTTON_SWITCH] = BUTTON_PIN_SWITCH;

    // настраиваем кнопку переключения частоты мигания светодиода
    const uint BUTTON_PIN_FREQUENCY = 11;
    platform_button_init(BUTTON_PIN_FREQUENCY);
    platform_buttons[BUTTON_FREQUENCY] = BUTTON_PIN_FREQUENCY;
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on);
}

bool platform_led_get(void)
{
    return gpio_get(LED_PIN);
}

bool platform_button_read(platform_button_t button_id)
{
    if (button_id == BUTTONS_COUNT)
        return false;

    return gpio_get(platform_buttons[button_id]);
}
