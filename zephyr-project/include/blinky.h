#ifndef BLINKY_H
#define BLINKY_H

#include <zephyr/kernel.h>

/* Blink interval in milliseconds */
#define BLINK_INTERVAL_MS 1000

/**
 * @brief Initialize LED subsystem
 * 
 * This function is called automatically during system initialization.
 * 
 * @return 0 on success, negative error code on failure
 */
int blinky_init(void);

#endif /* BLINKY_H */