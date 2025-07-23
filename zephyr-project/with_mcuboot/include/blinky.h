#ifndef BLINKY_H
#define BLINKY_H

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LED blinking
 * 
 * @return 0 on success, negative error code on failure
 */
int blinky_init(void);

/**
 * @brief Set LED blink interval
 * 
 * @param interval_ms Blink interval in milliseconds
 */
void blinky_set_interval(uint32_t interval_ms);

/**
 * @brief Set LED state
 * 
 * @param state 0 for off, 1 for on
 */
void blinky_set_state(int state);

#ifdef __cplusplus
}
#endif

#endif /* BLINKY_H */