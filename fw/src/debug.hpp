#pragma once

#include <WiFiUdp.h>

#include <string>

std::string device_name;

void debug(const std::string& msg) {
    WiFiUDP udp;
    udp.beginPacket({255, 255, 255, 255}, 16383);
    print(udp, "{}: {}\n", device_name, msg);
    udp.endPacket();
}

std::string show_whites(const std::string& str) {
    std::string res;
    for (char c: str) {
        switch(c) {
            case '\0': res += "\\0"; break;
            case '\a': res += "\\n"; break;
            case '\b': res += "\\b"; break;
            case '\f': res += "\\f"; break;
            case '\n': res += "\\n"; break;
            case '\r': res += "\\r"; break;
            case '\t': res += "\\t"; break;
            case '\v': res += "\\v"; break;
            case ' ' ... '~': res += c; break;
            default: res += format("\\x{:02x}", c);
        }
    }
    return res;
}
