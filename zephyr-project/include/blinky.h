#ifndef BLINKY_H
#define BLINKY_H

#include <zephyr/kernel.h>

typedef enum {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK_NORMAL,
    LED_MODE_BLINK_FAST,
    LED_MODE_BLINK_SLOW
} led_mode_t;

typedef enum {
    LED_STATUS_IDLE,
    LED_STATUS_WIFI_CONNECTING,
    LED_STATUS_WIFI_CONNECTED,
    LED_STATUS_OTA_CHECKING,
    LED_STATUS_OTA_DOWNLOADING,
    LED_STATUS_ERROR
} led_status_t;

/**
 * @brief Set LED mode
 * @param mode LED mode to set
 */
void led_set_mode(led_mode_t mode);

/**
 * @brief Set LED status (changes mode automatically)
 * @param status Application status
 */
void led_set_status(led_status_t status);

/**
 * @brief Get current LED mode
 * @return Current LED mode
 */
led_mode_t led_get_mode(void);

/**
 * @brief Initialize LED subsystem
 * 
 * This function is called automatically during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int blinky_init(void);

#endif /* BLINKY_H */