#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <string>
#include <format.h>

namespace wifi
{

std::string enctype2str(wifi_auth_mode_t auth) {
    switch (auth) {
        case WIFI_AUTH_OPEN: return "open";
        case WIFI_AUTH_WEP: return "WEP";
        case WIFI_AUTH_WPA_PSK: return "WPA";
        case WIFI_AUTH_WPA2_PSK: return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK: return "WPA_WPA2";
        case WIFI_AUTH_WPA2_ENTERPRISE : return "WPA2_ENTERPRISE";
        default: return fmt::format("Unknown[{}]", static_cast<int>(auth));
    }
}

template <class Stream>
void print_available(Stream& debug) {
    const int networks = WiFi.scanNetworks();
        for (int i = 0; i < networks; ++i) {
            print(debug, "\t{:32}\t{:15}\t{:4} dBm\n", WiFi.SSID(i).c_str(), enctype2str(WiFi.encryptionType(i)), WiFi.RSSI(i));
        }
}

void print_available() { print_available(Serial); }

template <class Stream>
bool connect(const char* SSID, const char* PSWD, Stream& debug) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    uint8_t cnt = 0;
    bool connected = false;
    std::string msg;
    while (cnt++ < 4) {
        print(debug, "\nConnecting to {} ...\n", SSID);
        int status = WiFi.begin(SSID, PSWD);
        for (uint8_t i = 0; i < 10; ++i) {
            delay(1000);
            switch (status) {
                case WL_CONNECTED:
                    msg = ("Connected");
                    cnt = 254;
                    i = 254;
                    connected = true;
                    break;
                case WL_NO_SHIELD:
                    msg = ("Heh. This is really weird, it looks as if your ESP did not have WiFi.");
                    cnt = 254;
                    i = 254;
                    break;
                case WL_IDLE_STATUS:
                    break;
                case WL_NO_SSID_AVAIL:
                    msg = ("I see no WiFi networks.");
                    break;
                case WL_SCAN_COMPLETED:
                    msg = ("Hmm, WiFi scan is done. But I did not start any scan.");
                    break;
                case WL_CONNECT_FAILED:
                    msg = ("Connection failed.");
                    break;
                case WL_CONNECTION_LOST:
                    msg = ("Connection lost. Never mind it has not been started yet.");
                    break;
                case WL_DISCONNECTED:
                    msg = ("Disconnected.");
                    break;
                default:
                    msg = fmt::format("Unknown status {}.", status);
                    break;
            }
            if (msg != "Disconnected." && msg != "Connected") {
                debug.print("Status #");
                debug.print(i);
                debug.print(": ");
                debug.println(msg.c_str());
            }
            status = WiFi.status();
        }
    }
    return connected;
}

bool connect(const char* SSID, const char* PSWD) { return connect(SSID, PSWD, Serial); }

// struct wifi_credentials_t {
//     const char* const SSID;
//     const char* const PSWD;
// };

// #define KNOWN_NETWORKS 1
// static const wifi_credentials_t wifi_credentials[KNOWN_NETWORKS] = {
//     { "SSID",       "PSWD"     }
// };
//
// bool connect() {
//     Serial.println("Scanning for known networks: ...");
//     const int networks = WiFi.scanNetworks();
//     int nt = 0;
//     for (int i = 0; i < networks; ++i) {
//         String ssid = WiFi.SSID(i);
//         for (nt = 0; nt != KNOWN_NETWORKS; ++nt) {
//             if (ssid == wifi_credentials[nt].SSID) {
//                 print(Serial, "Triing to connect to {} ...\n", wifi_credentials[nt].SSID);
//                 if (connect(Serial, wifi_credentials[nt].SSID, wifi_credentials[nt].PSWD))
//                     return true;
//                 break;
//             }
//         }
//         print(Serial, "Unknown network {}\n", ssid.c_str());
//     }
//     Serial.println("Fallback to hardtry connecting, because of hidden SSIDs: ...");
//     for (int i = 0; i != KNOWN_NETWORKS; ++i) {
//         print(Serial, "Triing to connect to {} ...\n", wifi_credentials[i].SSID);
//         if (connect(Serial, wifi_credentials[i].SSID, wifi_credentials[i].PSWD))
//             return true;
//     }
//     return false;
// }

} // namespace wifi
