#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "blinky.h"
#include "app_config.h"

LOG_MODULE_REGISTER(blinky, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Work item for LED control */
static struct k_work_delayable blink_work;
static uint8_t led_state = 0;
static uint32_t blink_interval_ms = LED_BLINK_INTERVAL_MS;

static void blink_work_handler(struct k_work *work)
{
    ARG_UNUSED(work);
    
    /* Toggle LED */
    gpio_pin_set_dt(&led, led_state);
    led_state ^= 1;
    
    /* Reschedule with current interval */
    k_work_schedule(&blink_work, K_MSEC(blink_interval_ms));
}

int blinky_init(void)
{
    if (!device_is_ready(led.port)) {
        LOG_ERR("LED device not ready");
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED GPIO: %d", ret);
        return ret;
    }

    /* Initialize work */
    k_work_init_delayable(&blink_work, blink_work_handler);
    
    /* Start blinking */
    k_work_schedule(&blink_work, K_MSEC(blink_interval_ms));

    LOG_INF("Blinky subsystem initialized - blinking every %d ms", blink_interval_ms);
    return 0;
}

void blinky_set_interval(uint32_t interval_ms)
{
    if (interval_ms == 0) {
        interval_ms = LED_BLINK_INTERVAL_MS;
    }
    
    if (blink_interval_ms != interval_ms) {
        LOG_INF("Changing blink interval from %d ms to %d ms", 
               blink_interval_ms, interval_ms);
        
        blink_interval_ms = interval_ms;
        
        /* Cancel current work and reschedule with new interval */
        k_work_cancel_delayable(&blink_work);
        k_work_schedule(&blink_work, K_MSEC(blink_interval_ms));
    }
}

void blinky_set_state(int state)
{
    if (state >= 0) {
        led_state = state ? 1 : 0;
        gpio_pin_set_dt(&led, led_state);
    }
}

/* Initialize at APPLICATION level */
SYS_INIT(blinky_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);