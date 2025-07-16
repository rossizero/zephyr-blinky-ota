#include "watchdog_manager.h"
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>
#include <stdatomic.h>

LOG_MODULE_REGISTER(wdt_mgr, LOG_LEVEL_INF);

#define MAX_MONITORED_TASKS 4 // Maximale Anzahl an Tasks, die überwacht werden können

static const struct device *wdt_dev;
static int wdt_channel_id;

static atomic_uint g_task_check_ins[MAX_MONITORED_TASKS];
static atomic_int g_registered_tasks_count = ATOMIC_VAR_INIT(0);

static struct k_timer wdt_mgr_timer;

/*
 * This is the core logic. It's a timer callback that runs periodically.
 */
static void watchdog_manager_check_tasks(struct k_timer *timer)
{
    bool all_tasks_ok = true;
    int task_count = atomic_load(&g_registered_tasks_count);

    for (int i = 0; i < task_count; i++) {
        // Exchange the flag: check if it was 1, and set it back to 0 atomically.
        if (atomic_exchange(&g_task_check_ins[i], 0) == 0) {
            // This task did NOT check in since the last cycle.
            LOG_ERR("Watchdog: Task with handle %d failed to check in!", i);
            all_tasks_ok = false;
        }
    }

    if (all_tasks_ok) {
        if (wdt_dev) {
            wdt_feed(wdt_dev, wdt_channel_id);
            LOG_DBG("All %d tasks checked in. Watchdog fed.", task_count);
        }
    } else {
        LOG_WRN("Watchdog not fed due to missing check-in. System will reset soon.");
        // We do nothing, letting the hardware watchdog time out.
    }
}

int watchdog_manager_init(k_timeout_t check_interval)
{
    wdt_dev = DEVICE_DT_GET(DT_ALIAS(watchdog0));
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("Hardware watchdog device not found or not ready.");
        return -ENODEV;
    }

    struct wdt_timeout_cfg wdt_config = {
        .window.min = 0,
        .window.max = 10000, // 10-Sekunden-Hardware-Timeout
        .callback = NULL,
        .flags = WDT_FLAG_RESET_SOC,
    };
    wdt_channel_id = wdt_install_timeout(wdt_dev, &wdt_config);
    if (wdt_channel_id < 0) {
        LOG_ERR("Failed to install watchdog timeout: %d", wdt_channel_id);
        return wdt_channel_id;
    }
    wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);

    // Initialisiere und starte unseren periodischen Software-Check-Timer
    k_timer_init(&wdt_mgr_timer, watchdog_manager_check_tasks, NULL);
    k_timer_start(&wdt_mgr_timer, check_interval, check_interval);

    LOG_INF("Watchdog Manager initialized. Check interval: %u ms, HW timeout: %u ms",
            check_interval.ticks, wdt_config.window.max);

    return 0;
}

int watchdog_manager_register_task(void)
{
    int handle = atomic_fetch_add(&g_registered_tasks_count, 1);
    if (handle >= MAX_MONITORED_TASKS) {
        atomic_store(&g_registered_tasks_count, MAX_MONITORED_TASKS); // Korrigiere den Zähler
        LOG_ERR("Cannot register more tasks. Limit of %d reached.", MAX_MONITORED_TASKS);
        return -ENOMEM;
    }

    // Initialisiere das Check-in-Flag auf 0. Der Task muss sofort einchecken.
    atomic_store(&g_task_check_ins[handle], 0);
    LOG_INF("Task registered with handle %d", handle);
    return handle;
}

void watchdog_manager_check_in(int task_handle)
{
    if (task_handle < 0 || task_handle >= MAX_MONITORED_TASKS) {
        return; // Ungültiges Handle
    }
    atomic_store(&g_task_check_ins[task_handle], 1);
}