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

/* LED state */
static struct {
    led_mode_t current_mode;
    led_status_t current_status;
    uint8_t led_state;
    uint32_t blink_interval;
} led_ctx = {
    .current_mode = LED_MODE_BLINK_NORMAL,
    .current_status = LED_STATUS_IDLE,
    .led_state = 0,
    .blink_interval = 1000
};

static void blink_work_handler(struct k_work *work)
{
    switch (led_ctx.current_mode) {
    case LED_MODE_OFF:
        gpio_pin_set_dt(&led, 0);
        /* Don't reschedule for OFF mode */
        return;
        
    case LED_MODE_ON:
        gpio_pin_set_dt(&led, 1);
        /* Don't reschedule for ON mode */
        return;
        
    case LED_MODE_BLINK_NORMAL:
    case LED_MODE_BLINK_FAST:
    case LED_MODE_BLINK_SLOW:
        /* Toggle LED */
        gpio_pin_set_dt(&led, led_ctx.led_state);
        led_ctx.led_state ^= 1;
        
        /* Reschedule with current interval */
        k_work_schedule(&blink_work, K_MSEC(led_ctx.blink_interval));
        break;
    }
}

static void update_blink_interval(void)
{
    switch (led_ctx.current_mode) {
    case LED_MODE_BLINK_FAST:
        led_ctx.blink_interval = LED_FAST_BLINK_MS;
        break;
    case LED_MODE_BLINK_NORMAL:
        led_ctx.blink_interval = LED_BLINK_INTERVAL_MS;
        break;
    case LED_MODE_BLINK_SLOW:
        led_ctx.blink_interval = LED_SLOW_BLINK_MS;
        break;
    default:
        led_ctx.blink_interval = LED_BLINK_INTERVAL_MS;
        break;
    }
}

void led_set_mode(led_mode_t mode)
{
    if (led_ctx.current_mode != mode) {
        led_ctx.current_mode = mode;
        update_blink_interval();
        
        /* Cancel current work and start new one */
        k_work_cancel_delayable(&blink_work);
        k_work_schedule(&blink_work, K_NO_WAIT);
    }
}

void led_set_status(led_status_t status)
{
    if (led_ctx.current_status != status) {
        led_ctx.current_status = status;
        
        /* Map status to LED mode */
        switch (status) {
        case LED_STATUS_IDLE:
            led_set_mode(LED_MODE_BLINK_SLOW);
            break;
        case LED_STATUS_WIFI_CONNECTING:
            led_set_mode(LED_MODE_BLINK_FAST);
            break;
        case LED_STATUS_WIFI_CONNECTED:
            led_set_mode(LED_MODE_BLINK_NORMAL);
            break;
        case LED_STATUS_OTA_CHECKING:
            led_set_mode(LED_MODE_BLINK_NORMAL);
            break;
        case LED_STATUS_OTA_DOWNLOADING:
            led_set_mode(LED_MODE_BLINK_FAST);
            break;
        case LED_STATUS_ERROR:
            led_set_mode(LED_MODE_ON);
            break;
        }
    }
}

led_mode_t led_get_mode(void)
{
    return led_ctx.current_mode;
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
    
    /* Start with idle status */
    led_set_status(LED_STATUS_IDLE);

    LOG_INF("Blinky subsystem initialized");
    return 0;
}

/* Auto-initialize at APPLICATION level */
SYS_INIT(blinky_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);