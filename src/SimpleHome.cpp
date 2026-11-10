#include <WiFi.h>
#include <time.h>

#include "esp_sntp.h"
#include "esp_idf_version.h"
#include "esp_arduino_version.h"

#include "SimpleHome.h"

/* Log WiFi events so that we can keep track of them */
void SimpleHome::_wifi_log_event(uint8_t reason) {
    WiFiLogEntry e;
    e.time = time(nullptr);
    e.uptime_ms = now_millis();
    e.reason = reason;
    portENTER_CRITICAL(&_wifi_log_mux);
    _wifi_log.push(e);
    portEXIT_CRITICAL(&_wifi_log_mux);
}

/* Handler for serving up the /restart page */
void SimpleHome::_handle_restart() {
    if( server.method() != HTTP_POST ) {
        // GET (or anything else): show the confirmation button
        String body = String(PAGE_HEAD);
        body += "<form method=\"post\" action=\"/restart\">\n";
        body += " <button type=\"submit\">Restart %TITLE%</button>\n";
        body += "</form>\n";
        body += String(PAGE_FOOT);
        body.replace("%TITLE%", this->name());
        
        server.send(200, "text/html", body);
    } else {
        if( !server.authenticate(_auth_user, _auth_pass)) {
            return server.requestAuthentication(BASIC_AUTH, this->name());
        }

        this->after(500, delayed_restart_esp);

        String body = String(PAGE_HEAD);
        body.replace("%TITLE%", this->name());
        body += "Restarting in 5 s...<br>\n";
        body += String(PAGE_FOOT);
        
        server.send(200, "text/html", body);
    }
}

/* Handler for serving up the /identify.json page */
void SimpleHome::_handle_identify_json() {
    char core_ver[16];
    snprintf(core_ver, sizeof(core_ver), "%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    
    JSONObject body;
    body.add("name", this->name());
    body.add("simplehome", SH_VERSION);
    body.add("mac", WiFi.macAddress());
    //body.add("ip", WiFi.localIP().toString());
    body.add("board", ARDUINO_BOARD);
    body.add("variant", ARDUINO_VARIANT);
    body.add("target", CONFIG_IDF_TARGET);
    body.add("chip", ESP.getChipModel());
    body.add("chip_revision", ESP.getChipRevision());
    body.add("cores", ESP.getChipCores());
    body.add("cpu_mhz", ESP.getCpuFreqMHz());
    body.add("flash_bytes", ESP.getFlashChipSize());
    body.add("arduino_core", core_ver);
    body.add("idf", ESP.getSdkVersion());
    //body.add("sketch_md5", ESP.getSketchMD5());
    body.add("built", __DATE__ " " __TIME__);

    server.send(200, "application/json", body.render());
}

/* Handler for serving up the /status page */
void SimpleHome::_handle_status() {
    float uptime = get_uptime();
    const char* status = get_watchdog_status();
    const char* reason = get_last_reboot_reason();
    time_t synced = get_last_time_sync();
    MemoryInfo mem = get_memory_usage();
    
    float tsoc = get_soc_temp();
    long rssi = WiFi.RSSI();
    
    portENTER_CRITICAL(&_wifi_log_mux);
    RingBuffer<WiFiLogEntry, SH_WIFI_LOG_SIZE> wifi_log = _wifi_log;
    portEXIT_CRITICAL(&_wifi_log_mux);
    
    String body = String(PAGE_HEAD);
    body.replace("%TITLE%", this->name());
    TwoColumnTable table;
    table.add_row("Uptime", String(uptime));
    table.add_row("Last reboot", reason);
    table.add_row("Watchdog", status);
    table.add_row("Last time sync", synced ? format_epoch(synced, "%Y/%m/%d %H:%M:%S") : String("never"));
    table.add_row("Free heap size", mem.free_heap, "B");
    table.add_row("Min free heap size", mem.min_free_heap, "B");
    table.add_row("Heap fragmentation", mem.fragmentation*100.0, "%");
    table.add_row("SoC temperature", tsoc, "C");
    table.add_row("WiFi signal strength", rssi, "dBm");
    table.add_row("WiFi connects", _wifi_nconnect);
    table.add_row("WiFi disconnects", _wifi_ndisconnect);
    //for(const WiFiLogEntry &e : wifi_log) {
    //    table.add_row(format_epoch(e.time, "%Y/%m/%d %H:%M:%S"), e.reason ? "disconnected, reason " + String(e.reason) : String("connected"));
    //}
    table.add_row("Updated", format_epoch(time(nullptr), "%Y/%m/%d %H:%M:%S"));
    body += table;
    body += String(PAGE_FOOT);
    
    server.send(200, "text/html", body);
}

/* Handler for serving up the /status.json page (JSON version of /status) */
void SimpleHome::_handle_status_json() {
    float uptime = get_uptime();
    const char* status = get_watchdog_status();
    const char* reason = get_last_reboot_reason();
    time_t synced = get_last_time_sync();
    MemoryInfo mem = get_memory_usage();
    
    float tsoc = get_soc_temp();
    long rssi = WiFi.RSSI();
    
    portENTER_CRITICAL(&_wifi_log_mux);
    RingBuffer<WiFiLogEntry, SH_WIFI_LOG_SIZE> wifi_log = _wifi_log;
    portEXIT_CRITICAL(&_wifi_log_mux);
    
    JSONObject body;
    body.add("uptime", uptime);
    body.add("last_reboot", reason);
    body.add("watchdog", status);
    body.add("last_time_sync", synced ? format_epoch(synced, "%Y/%m/%d %H:%M:%S") : String("never"));
    body.add("free_heap", mem.free_heap);
    body.add("min_free_heap", mem.min_free_heap);
    body.add("heap_fragmentation", mem.fragmentation);
    body.add("temp_soc", tsoc);
    body.add("wifi_signal_strength", rssi);
    body.add("wifi_connects", _wifi_nconnect);
    body.add("wifi_disconnects", _wifi_ndisconnect);
    body.add("updated", format_epoch(time(nullptr), "%Y/%m/%d %H:%M:%S"));
    //JSONArray wifi_events;
    //for(const WiFiLogEntry &e : wifi_log) {
    //    JSONObject entry;
    //    entry.add("time", format_epoch(e.time, "%Y/%m/%d %H:%M:%S"));
    //    entry.add("uptime_ms", (long long) e.uptime_ms);
    //    entry.add("reason", (int) e.reason);
    //    wifi_events.add(entry);
    //}
    //body.add("wifi_log", wifi_events);
    
    server.send(200, "application/json", body.render());
}

/* Add a new periodic task to the task pool */
void SimpleHome::every(uint32_t period_ms, void (*fnc)()) {
    if( _ntask < SH_MAX_NTASK ) {
        _tasks[_ntask++] = {period_ms, 0, fnc};
    }
}

/* Add a new single shot event to the event pool */
void SimpleHome::after(uint32_t wait_ms, void (*fnc)()) {
    for(int i=0; i<SH_MAX_NEVENT; i++) {
        if( !_events[i].active ) {
            _events[i] = {now_millis() + wait_ms, 1, fnc};
            _nevent++;
            break;
        }
    }
}

/* setup() function to help bring up SimpleHome and set its default functionality */
void SimpleHome::begin() {
    // Setup serial for debugging on the bench
    Serial.begin(115200);
    
    // Setup the WiFi
    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        _wifi_ndisconnect++;
        _wifi_log_event(info.wifi_sta_disconnected.reason);
    }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
    WiFi.onEvent([this](arduino_event_id_t event, arduino_event_info_t info) {
        _wifi_nconnect++;
        _wifi_log_event(0);
    }, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  
    WiFi.persistent(false);
    if( _static_ip ) {
        WiFi.config(_ip, _gateway, _subnet, _dns1, _dns2);
    }
    WiFi.begin(_ssid, _pass);
    WiFi.setSleep(WIFI_PS_MAX_MODEM);
    delay(10);
    
    // Setup the watchdog for (SH_WD_TIMEOUT_MS / 1000) seconds
    start_watchdog(SH_WD_TIMEOUT_MS);
    
    // Start the web server
    server.begin();
    
    // Register the default pages
    server.on("/restart", [this]() { _handle_restart(); });
    server.on("/identify.json", [this]() { _handle_identify_json(); });
    server.on("/status", [this]() { _handle_status(); });
    server.on("/status.json", [this]() { _handle_status_json(); });
    if( (_icon != nullptr) && (_icon_size > 0) ) {
        server.on("/favicon.ico", [this]() { _handle_favicon(); });
    }
}

/* loop() function to run SimpleHomes per iteration calls */
void SimpleHome::loop() {
    uint64_t now;
    
    // Feed the watchdog
    esp_task_wdt_reset();
    
    // WiFi check
    now = now_millis();
    if( WiFi.status() == WL_CONNECTED ) {
        // It's up
        _wifi_last_ok = now;
        _wifi_nattempt = 0;
        
        if( !esp_sntp_enabled() ) {
            // Connect to a NTP server to set the RTC and have SNTP re-sync it daily
            sntp_set_time_sync_notification_cb(_on_ntp_sync);
            sntp_set_sync_interval(86400*1000);
            configTime(0, 0, "0.north-america.pool.ntp.org", "1.north-america.pool.ntp.org", "time.nist.gov");
            struct tm timeinfo;
            getLocalTime(&timeinfo);
        }
        
        server.handleClient();
    } else {
        // It's down
        if( (now - _wifi_last_attempt) >= 30000) {
            _wifi_last_attempt = now;
            _wifi_nattempt++;
            
            if( _wifi_nattempt % 5 == 0) {
                // Try a radio reset if we've failed many times
                WiFi.mode(WIFI_OFF);
                delay(100);
                WiFi.mode(WIFI_STA);
            }
            
            // Disconnect and reconnect
            WiFi.disconnect();
            if( _static_ip ) {
                WiFi.config(_ip, _gateway, _subnet, _dns1, _dns2);
            }
            WiFi.begin(_ssid, _pass);
            
            // Feed the watchdog to account for a slow connect
            esp_task_wdt_reset();
        }
    }
    
    for(int i=0; i<_ntask; i++) {
        if( (now - _tasks[i].last) >= _tasks[i].period_ms) {
            _tasks[i].last = now;
            _tasks[i].fnc();
            
            // Feed the watchdog again to account for a slow task
            esp_task_wdt_reset();
        }
    }
    
    
    if( _nevent ) {
        int nprocessed = 0;
        for(int i=0; i<SH_MAX_NEVENT; i++) {
            if( _events[i].active && (now >= _events[i].when) ) {
                _events[i].fnc();
                _events[i].active = 0;
                nprocessed++;
                
                // Feed the watchdog again to account for a slow event
                esp_task_wdt_reset();
            }
        }
        _nevent -= nprocessed;
    }
    
    // Yield so the CPU can idle between loop calls
    delay(10);
}
