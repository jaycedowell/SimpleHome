#pragma once

#include <Arduino.h>
#include <cmath>
#include <type_traits>
#include <cstddef>
#include <stdio.h>

/* Format a float with the specified precision, unless it's not finite and then it's null */
template<typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
inline String format_number_null(T value, unsigned int prec, const char *fallback) {
    if( !std::isfinite(value) ) {
        return String("null");
    }
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%.*f", prec, value);
    if( (n <= 0) || (n >= (int) sizeof(buf)) ) {
        return String(fallback);
    }
    return String(buf);
}

/* Quote a string so that it can go into JSON */
inline String json_quote(const String &text) {
    String out;
    out.reserve(text.length() + 2);
    out += '"';
    for(unsigned int i=0; i<text.length(); i++) {
        char c = text[i];
        switch(c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if( (unsigned char) c < 0x20 ) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", (unsigned int) (unsigned char) c);
                    out += buf;
                } else {
                    out += c;
                }
        }
    }
    out += '"';
    return out;
}

class JSONObject;
class JSONArray;

/* A single JSON value */
class JSONValue {
    String _text;
public:
    JSONValue(std::nullptr_t) : _text("null") {}
    JSONValue(bool value) : _text(value ? "true" : "false") {}
    JSONValue(const char *value) : _text(value != nullptr ? json_quote(String(value)) : String("null")) {}
    JSONValue(char *value) : JSONValue((const char *) value) {}
    JSONValue(const String &value) : _text(json_quote(value)) {}
    JSONValue(int value) : _text(value) {}
    JSONValue(unsigned int value) : _text(value) {}
    JSONValue(long value) : _text(value) {}
    JSONValue(unsigned long value) : _text(value) {}
    JSONValue(long long value) : _text(value) {}
    JSONValue(unsigned long long value) : _text(value) {}
    JSONValue(float value, unsigned int prec = 2) : _text(format_number_null(value, prec, "null")) {}
    JSONValue(double value, unsigned int prec = 2) : _text(format_number_null(value, prec, "null")) {}
    JSONValue(const JSONObject &value);
    JSONValue(const JSONArray &value);
    template <typename T>
    JSONValue(T *value) = delete;

    inline const String& text() const { return _text; }
    inline String nested() const {
        String out = _text;
        out.replace("\n", "\n ");
        return out;
    }
};

/* A JSON object/dictionary */
class JSONObject {
    String _items;
public:
    inline void add(const String &key, const JSONValue &value) {
        if( _items.length() > 0 ) {
            _items += ",\n";
        }
        _items += " " + json_quote(key) + ": " + value.nested();
    }
    inline void add(const String &key, double value, unsigned int prec) {
        add(key, JSONValue(value, prec));
    }
    inline String render() const {
        if( _items.length() == 0 ) {
            return String("{}");
        }
        return "{\n" + _items + "\n}";
    }
    inline operator String() const { return render(); }
};

/* A JSON array/list */
class JSONArray {
    String _items;
public:
    inline void add(const JSONValue &value) {
        if( _items.length() > 0 ) {
            _items += ",\n";
        }
        _items += " " + value.nested();
    }
    inline void add(double value, unsigned int prec) {
        add(JSONValue(value, prec));
    }
    inline String render() const {
        if( _items.length() == 0 ) {
            return String("[]");
        }
        return "[\n" + _items + "\n]";
    }
    inline operator String() const { return render(); }
};

inline JSONValue::JSONValue(const JSONObject &value) : _text(value.render()) {}
inline JSONValue::JSONValue(const JSONArray &value) : _text(value.render()) {}
