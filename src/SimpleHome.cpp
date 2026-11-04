#include <WiFi.h>
#include <time.h>

#include "esp_sntp.h"
#include "esp_idf_version.h"
#include "esp_arduino_version.h"

#include "SimpleHome.h"

/* Log WiFi events so that we can keep track of them */
void SimpleHome::_wifi_log_event(uint8_t reason) {
    WiFiLogEntry &e = _wifi_log[_wifi_log_count % SH_WIFI_LOG_SIZE];
    e.time = time(nullptr);
    e.uptime_ms = now_millis();
    e.reason = reason;
    _wifi_log_count++;
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
        
        String body = String(PAGE_HEAD);
        body.replace("%TITLE%", this->name());
        body += "Restarting in 5 s...<br>\n";
        body += String(PAGE_FOOT);
        server.send(200, "text/html", body);
        
        delay(500);
        
        delayed_restart_esp();
    }
}

/* Handler for serving up the /identify.json page */
void SimpleHome::_handle_identify_json() {
    char core_ver[16];
    snprintf(core_ver, sizeof(core_ver), "%d.%d.%d", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    
    String body = String("{\n");
    body += " \"name\": \"" + String(this->name()) + "\",\n";
    body += " \"simplehome\": \"" SH_VERSION "\",\n";
    body += " \"mac\": \"" + WiFi.macAddress() + "\",\n";
    //body += " \"ip\": \"" + WiFi.localIP().toString() + "\",\n";
    body += " \"board\": \"" ARDUINO_BOARD "\",\n";
    body += " \"variant\": \"" ARDUINO_VARIANT "\",\n";
    body += " \"target\": \"" CONFIG_IDF_TARGET "\",\n";
    body += " \"chip\": \"" + String(ESP.getChipModel()) + "\",\n";
    body += " \"chip_revision\": " + String(ESP.getChipRevision()) + ",\n";
    body += " \"cores\": " + String(ESP.getChipCores()) + ",\n";
    body += " \"cpu_mhz\": " + String(ESP.getCpuFreqMHz()) + ",\n";
    body += " \"flash_bytes\": " + String(ESP.getFlashChipSize()) + ",\n";
    body += " \"arduino_core\": \"" + String(core_ver) + "\",\n";
    body += " \"idf\": \"" + String(ESP.getSdkVersion()) + "\",\n";
    //body += " \"sketch_md5\": \"" + ESP.getSketchMD5() + "\",\n";
    body += " \"built\": \"" __DATE__ " " __TIME__ "\"\n";
    body += "}\n";
    
    server.send(200, "application/json", body);
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
    
    String body = String(PAGE_HEAD);
    body.replace("%TITLE%", this->name());
    body += "Uptime: " + String(uptime) + "<br>\n";
    body += "Last reboot: " + String(reason) + "<br>\n";
    body += "Watchdog: " + String(status) + "<br>\n";
    body += "Last time sync: " + (synced ? format_epoch(synced, "%Y/%m/%d %H:%M:%S") : String("never")) + "<br>\n";
    body += "<br>\n";
    body += "Free heap size: " + String(mem.free_heap) + "B<br>\n";
    body += "Min free heap size: " + String(mem.min_free_heap) + "B<br>\n";
    body += "Heap fragmentation: " + String(mem.fragmentation*100.0) + "%<br>\n";
    body += "<br>\n";
    body += "SoC temperature: " + String(tsoc) + " C<br>\n";
    body += "WiFi signal strength: " + String(rssi) + " dBm<br>\n";
    body += "WiFi connects: " + String(_wifi_nconnect) + "<br>\n";
    body += "WiFi disconnects: " + String(_wifi_ndisconnect) + "<br>\n";
    body += "<br>\n";
    body += "Updated: " + format_epoch(time(nullptr), "%Y/%m/%d %H:%M:%S") + "<br>\n";
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
    
    String body = String("{\n");
    body += " \"uptime\": " + String(uptime) + ",\n";
    body += " \"last_reboot\": \"" + String(reason) + "\",\n";
    body += " \"watchdog\": \"" + String(status) + "\",\n";
    body += " \"last_time_sync\": \"" + (synced ? format_epoch(synced, "%Y/%m/%d %H:%M:%S") : String("never")) + "\",\n";
    body += " \"free_heap\": " + String(mem.free_heap) + ",\n";
    body += " \"min_free_heap\": " + String(mem.min_free_heap) + ",\n";
    body += " \"heap_fragmentation\": " + String(mem.fragmentation) + ",\n";
    body += " \"temp_soc\": " + String(tsoc) + ",\n";
    body += " \"wifi_signal_strength\": " + String(rssi) + ",\n";
    body += " \"wifi_connects\": " + String(_wifi_nconnect) + ",\n";
    body += " \"wifi_disconnects\": " + String(_wifi_ndisconnect) + ",\n";
    body += " \"updated\": \"" + format_epoch(time(nullptr), "%Y/%m/%d %H:%M:%S") + "\"\n";
    body += "}";
    
    server.send(200, "application/json", body);
}

/* Add a new periodic task to the task pool */
void SimpleHome::every(uint32_t period_ms, void (*fnc)()) {
    if( _ntask < SH_MAX_NTASK ) {
        _tasks[_ntask++] = {period_ms, 0, fnc};
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
            WiFi.config(staticIP, gateway, subnet, primaryDNS, secondaryDNS);
            WiFi.begin(ssid, pass);
            
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
    
    // Yield so the CPU can idle between loop calls
    delay(10);
}
