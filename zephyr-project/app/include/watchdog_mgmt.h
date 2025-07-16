#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include <zephyr/kernel.h>

/**
 * @brief Initializes the watchdog manager and the underlying hardware watchdog.
 *
 * This function should be called once at startup before any tasks
 * that need monitoring are started. It also starts a periodic timer
 * that checks the health of all registered tasks.
 *
 * @param check_interval The interval at which the manager checks task health.
 *                       Should be significantly shorter than the watchdog timeout.
 * @return 0 on success, or a negative error code on failure.
 */
int watchdog_manager_init(k_timeout_t check_interval);

/**
 * @brief Registers a task with the watchdog manager.
 *
 * Each critical task that needs to be monitored must register itself.
 * The manager returns a handle (ID) that the task must use to check in.
 *
 * @return A valid handle (>= 0) on success, or a negative error code if
 *         the manager is full or not initialized.
 */
int watchdog_manager_register_task(void);

/**
 * @brief Allows a registered task to "check in" or "pet" the manager.
 *
 * Tasks must call this periodically to signal that they are still alive
 * and functioning correctly.
 *
 * @param task_handle The handle received from watchdog_manager_register_task().
 */
void watchdog_manager_check_in(int task_handle);

#endif /* WATCHDOG_MANAGER_H */