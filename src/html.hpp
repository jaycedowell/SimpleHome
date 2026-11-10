#pragma once

#include <Arduino.h>
#include <cmath>
#include <stdio.h>
#include <type_traits>

// Page header with a %TITLE% template
static const char PAGE_HEAD[] = R"html(<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link href="/favicon.ico" rel="icon" type="image/x-icon">
    <title>%TITLE%</title>
    <style>
        :root { color-scheme: light dark; --bg: #eef2f5; --card: #fff; --text: #1f2a33; --muted: #5f6f7c; --line: #d9e1e7; --accent: #1f6f8b; }
        @media (prefers-color-scheme: dark) {
            :root { --bg: #12181d; --card: #1b242b; --text: #e4eaee; --muted: #9aa9b4; --line: #2e3a43; --accent: #5cb8d6; }
        }
        html { background: var(--bg); }
        body { font: 1rem/1.7 system-ui, sans-serif; font-variant-numeric: tabular-nums; color: var(--text); background: var(--card); max-width: 34em; margin: 1rem auto; padding: 1em 1.5em 1.5em; border-radius: .5em; }
        h1 { font-size: 1.4em; color: var(--accent); margin: 0 0 .6em; padding-bottom: .3em; border-bottom: 2px solid var(--accent); }
        a { color: var(--accent); }
        table { border-collapse: collapse; width: 100%; }
        th, td { padding: .3em 0; text-align: left; vertical-align: top; border-bottom: 1px solid var(--line); }
        th { color: var(--accent); padding-top: .8em; }
        tr:last-child td { border-bottom: 0; }
        td:first-child { padding-right: 1em; color: var(--muted); }
        button { font: inherit; color: var(--card); background: var(--accent); padding: .4em 1em; border: 0; border-radius: .3em; cursor: pointer; }
        nav { display: flex; gap: 1em; margin: -.3em 0 1em; }
        nav a.active { font-weight: 600; text-decoration: none; color: var(--text); }
    </style>
</head>
<body>
<h1>%TITLE%</h1>
)html";

// Page footer
static const char PAGE_FOOT[] = R"html(</body>
</html>
)html";

/* Format a float with the specified precision, unless it's not finite and then it's null */
template<typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
inline String format_number(T value, unsigned int prec) {
    if( !std::isfinite(value) ) {
        return String("null");
    }
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%.*f", prec, value);
    if( (n <= 0) || (n >= (int) sizeof(buf)) ) {
        return String("null");
    }
    return String(buf);
}

/* Class to help build tables that look nice */
class TwoColumnTable {
    String _rows;
public:
    inline void add_header(const String &title) {
        _rows += "<tr><th colspan=\"2\">" + title + "</th></tr>\n";
    }
    inline void add_row(const String &label, const String &value) {
        // String variant
        _rows += "<tr><td>" + label + "</td><td>" + value + "</td></tr>\n";
    }
    inline void add_row(const String &label, const char *value) {
        // C string variant (to keep pointers from converting to bool)
        add_row(label, String(value));
    }
    inline void add_row(const String &label, bool value) {
        // bool variant
        _rows += "<tr><td>" + label + "</td><td>" + String(value ? "true" : "false") + "</td></tr>\n";
    }
    template<typename T, typename = typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
    inline void add_row(const String &label, T value, const String &unit="") {
        // general numeric + unit variant
        _rows += "<tr><td>" + label + "</td><td>" + String(value);
        if( unit.length() ) {
            _rows += " " + unit;
        }
        _rows += "</td></tr>\n";
    }
    template<typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
    inline void add_row(const String &label, T value, unsigned int prec, const String &unit="") {
        // set precision floating + unit variant
        _rows += "<tr><td>" + label + "</td><td>" + format_number(value, prec);
        if( unit.length() ) {
            _rows += " " + unit;
        }
        _rows += "</td></tr>\n";
    }
    
    inline String render() const {
        return "<table>\n" + _rows + "</table>\n";
    }
    inline operator String() const { return render(); }
};

class HTMLPage {
    String _title;
    String _navigation;
    String _content;
public:
    HTMLPage(const String &title) : _title(title) {}
    inline void add_navigation_link(const String &page, const String &title, bool active=false) {
        _navigation += "<a href=\"" + page + "\"" + (active ? " class=\"active\"" : "") + ">" + title + "</a>";
    }
    inline void add_content(const String &content) {
        _content += content;
    }
    inline String render() const {
        String body = String(PAGE_HEAD);
        body.replace("%TITLE%", _title);
        if( _navigation.length() ) {
            body += "<nav>" + _navigation + "</nav>\n";
        }
        body += _content;
        body += String(PAGE_FOOT);
        return body;
    }
    inline operator String() const { return render(); }
};
