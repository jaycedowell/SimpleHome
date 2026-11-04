#pragma once

#include <time.h>

#include "esp_task_wdt.h"
#include "esp_idf_version.h"
#include "esp_sntp.h"

/* Let people know if we are only an old IDF core */
#if ESP_IDF_VERSION_MAJOR < 5
    #pragma message "SimpleHome: building on ESP-IDF 4.x core; watchdog fallbacks in use"
#endif

/*
 * Time Utilities
 */

/* Return the time of the last successful NTP sync, or 0 if never synced */
inline volatile time_t& get_last_time_sync() {
    static volatile time_t last_sync = 0;
    return last_sync;
}

/* Set the time stamp of the latest NTP sync */
inline void _on_ntp_sync(struct timeval *tv) {
    get_last_time_sync() = tv->tv_sec;
}

/* 64-bit version of the standard Arduino millis() function */
inline int64_t now_millis() {
    return esp_timer_get_time() / 1000;
}

/* Convert a Unix epoch into a YYYY/MM/DD HH:MM:SS string */
inline String format_epoch(unsigned long t, const char* fmt) {
    char buf[128];
    time_t tt = t;
    struct tm *ti = gmtime(&tt);
    strftime(buf, sizeof(buf), fmt, ti);
    return String(buf);
}

/* Poll the current time and convert it into a YYYY/MM/DD HH:MM:SS string */
inline String format_now(const char* fmt) {
    return format_epoch(time(nullptr), fmt);
}

/*
 * Temperature Utilities
 */

/* Return the chip temperature in C */
inline float get_soc_temp() {
    return temperatureRead();
}

/* Convert a temperature in C to F */
inline float temp_c_to_f(float temp) {
    return temp*9/5 + 32;
}

/* Convert a temperature in F to C */
inline float temp_f_to_c(float temp) {
    return (temp - 32)*5/9;
}

/*
 * ESP32 Utilities
 */

/* Get the current uptime in s */
inline float get_uptime() {
    uint64_t uptime = esp_timer_get_time();
    return uptime / 1e6;
}

/* Configure and start the watchdog */
inline void start_watchdog(uint32_t timeout_ms) {
#if ESP_IDF_VERSION_MAJOR >= 5
    esp_task_wdt_config_t cfg = {
        .timeout_ms = timeout_ms,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    if (esp_task_wdt_reconfigure(&cfg) == ESP_ERR_INVALID_STATE) {
        esp_task_wdt_init(&cfg);
    }
#else
    esp_task_wdt_init((timeout_ms + 999) / 1000, true);   // IDF 4.4: timeout in seconds
#endif
    esp_task_wdt_add(nullptr);
}

/* Get the current status of the watchdog as a String */
inline const char* get_watchdog_status() {
    esp_err_t wd_status = esp_task_wdt_status(nullptr);
    switch(wd_status) {
        case ESP_OK:                return "running";
        case ESP_ERR_NOT_FOUND:     return "not subscribed";
        case ESP_ERR_INVALID_STATE: return "not initialized";
        default:                    return "unknown";
    }
}

/* Get the reason for the last reboot as a String */
inline const char* get_last_reboot_reason() {
    esp_reset_reason_t rst_reason = esp_reset_reason();
    switch(rst_reason) {
        case ESP_RST_TASK_WDT:  // fall through
        case ESP_RST_INT_WDT:   // fall through
        case ESP_RST_WDT:       return "watchdog";
        case ESP_RST_POWERON:   return "power on";
        case ESP_RST_EXT:       return "external reset";
        case ESP_RST_SW:        return "software reset";
        case ESP_RST_DEEPSLEEP: return "wake from deep sleep";
        case ESP_RST_BROWNOUT:  return "brownout";
        case ESP_RST_PANIC:     return "panic";
#if ESP_IDF_VERSION_MAJOR >= 5
        case ESP_RST_USB:        return "USB reset";
        case ESP_RST_PWR_GLITCH: return "power glitch";
        case ESP_RST_CPU_LOCKUP: return "CPU lockup";
#endif
        default:                return "other";
    }
}

// Structure to hold info about the ESP32's memory
typedef struct _MemoryInfo {
    uint32_t free_heap;            // B
    uint32_t min_free_heap;        // B
    uint32_t max_free_heap_alloc;  // B
    float    fragmentation;        // fraction [0..1]
} MemoryInfo;

/* Get the current state of the ESP32's memory */
inline MemoryInfo get_memory_usage() {
    MemoryInfo info = {0};
    info.free_heap = ESP.getFreeHeap();
    info.min_free_heap = ESP.getMinFreeHeap();
    info.max_free_heap_alloc = ESP.getMaxAllocHeap();
    info.fragmentation = 1.0 - (float) info.max_free_heap_alloc / (float) info.free_heap;
    return info;
}

/* Reboot the microcontroller in 5 s */
inline void delayed_restart_esp() {
    uint64_t sleepTime = 5 * 1000000;
    esp_sleep_enable_timer_wakeup(sleepTime);
    esp_deep_sleep_start();
}
