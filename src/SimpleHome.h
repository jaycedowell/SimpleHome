#pragma once

#include <WiFi.h>
#include <WebServer.h>

#include "battery.hpp"
#include "html.hpp"
#include "json.hpp"
#include "ring_buffer.hpp"
#include "utils.hpp"

#define SH_VERSION "0.0.0"

#ifndef SH_MAX_NTASK
    #define SH_MAX_NTASK 8
#endif

#ifndef SH_MAX_NEVENT
    #define SH_MAX_NEVENT 8
#endif

#ifndef SH_WD_TIMEOUT_MS
    #define SH_WD_TIMEOUT_MS 45000
#endif

#ifndef SH_WIFI_LOG_SIZE
    #define SH_WIFI_LOG_SIZE 8
#endif

#ifndef SH_MAX_NLINK
    #define SH_MAX_NLINK 6
#endif

// Structure to hold a task that periodically runs
typedef struct _Task {
    uint32_t period_ms;
    int64_t last;
    void (*fnc)();
} Task;

// Structure to hold a single shot event
typedef struct _Event {
    uint64_t when;
    uint8_t active;
    void (*fnc)();
} Event;

// Structure to hold changes in the WiFi state
typedef struct _WiFiLogEntry {
    time_t   time;
    int64_t  uptime_ms;
    uint8_t  reason;
} WiFiLogEntry;

// Structure to hold a single link and the link's title
typedef struct _Link {
    const char* page;
    const char* title;
} Link;
 
class SimpleHome {
    const char *_name;
    const byte *_icon;
    size_t _icon_size;
    
    Task _tasks[SH_MAX_NTASK];
    int _ntask = 0;
    
    Event _events[SH_MAX_NEVENT] = {0};
    int _nevent = 0;
    
    uint64_t _wifi_last_ok;
    uint64_t _wifi_last_attempt;
    int32_t  _wifi_nattempt;
    volatile uint32_t _wifi_ndisconnect = 0;
    volatile uint32_t _wifi_nconnect = 0;
    RingBuffer<WiFiLogEntry, SH_WIFI_LOG_SIZE> _wifi_log;
    portMUX_TYPE _wifi_log_mux = portMUX_INITIALIZER_UNLOCKED;
    
    void _wifi_log_event(uint8_t reason);
    
    const char *_ssid = nullptr, *_pass = nullptr;
    const char *_auth_user = nullptr, *_auth_pass = nullptr;
    bool _static_ip = false;
    IPAddress _ip, _gateway, _subnet, _dns1, _dns2;
    
    Link _links[SH_MAX_NLINK];
    int _nlink = 0;
    
    void _handle_restart();
    void _handle_identify_json();
    void _handle_status();
    void _handle_status_json();
    inline void _handle_favicon() { server.send_P(200, "image/x-icon", (PGM_P)_icon, _icon_size); }
public:
    WebServer server{80};
    
    SimpleHome(const char* name, const byte* icon, size_t icon_size): _ntask(0), _nevent(0), _wifi_last_ok(0), _wifi_last_attempt(0), _wifi_nattempt(0) {
        _name = name;
        _icon = icon;
        _icon_size = icon_size;
    }
    inline const char* name() { return _name; }
    inline void set_wifi(const char *ssid, const char *pass) {
        _ssid = ssid;
        _pass = pass;
    }
    inline void set_static_ip(IPAddress ip, IPAddress gw, IPAddress sn, IPAddress dns1, IPAddress dns2) {
        _static_ip = true;
        _ip = ip;
        _gateway = gw;
        _subnet = sn;
        _dns1 =
        dns1; _dns2 = dns2;
    }
    inline void set_auth(const char *user, const char *pass) {
        _auth_user = user;
        _auth_pass = pass;
    }
    inline void register_navigation_link(const char *page, const char *title) {
        if( _nlink < SH_MAX_NLINK ) {
            _links[_nlink++] = {page, title};
        }
    }
    HTMLPage get_base_html_page();
    void every(uint32_t period_ms, void (*fnc)());
    void after(uint32_t wait_ms, void (*fnc)());
    void begin();
    void loop();
};
