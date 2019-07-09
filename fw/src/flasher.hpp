#include <sstream>

#include "wifi.hpp"

namespace flasher {

using controller::controller_ip;
std::string ID;

int output_port = 0;

void server_process(std::string input, WiFiClient& client) {
    debug(format("recvd from {}: {}", client.remoteIP().toString().c_str(), show_whites(input)));
    auto arg = split(input);
    if (arg.empty()) {
        debug("Empty arg!");
        return;
    }
    auto& cmd = arg[0];
    LED_MAP_t::const_iterator led = LED.find(cmd);
    if (led != LED.end()) {
        int brightness = 0;
        switch (arg.size()) {
        case 1:
            debug(format("Led {} without argument!\n", cmd));
            break;
        case 2:
            if (std::istringstream(arg[1])>>brightness)
                ledcWrite(led->second, brightness);
            else
                debug("Can not convert brightness to number");
            break;
        case 3:
        default:
            print(Serial, "Led {} set to {} for {} ms\n", cmd, arg[1], arg[2]);
            break;
        }
    } else if (cmd == CMD::power_next) {
        const uint8_t value = (arg.size() >= 2 && arg[1] != "0") ? HIGH : LOW;
        switch(output_port) {
            case 1: digitalWrite(PWR1_EN, value); break;
            case 2: digitalWrite(PWR2_EN, value); break;
        }
    } else if (cmd == CMD::id) {
        if (arg.size() >= 2) {
            ID = arg[1];
            debug(format("ID set to {}", ID));
        } else
            print(Serial, "ID without argument!");
    } else {
        print(Serial, "Unknown command \"{}\"\n", cmd);
    }
}

void setup() {
    device_name = format("Flasher {}", WiFi.macAddress().c_str());
    print(Serial, device_name + '\n');
    print(Serial, "Waiting for power up, skip by space...\n");
    for(;;) {
        if (digitalRead(PWR1_MEAS) == HIGH) {
            digitalWrite(PWR1_EN, HIGH);
            output_port = 2;
            break;
        } else if (digitalRead(PWR2_MEAS) == HIGH) {
            digitalWrite(PWR2_EN, HIGH);
            output_port = 1;
            break;
        }
        if (Serial.available()) {
            char c = Serial.read();
            if (c == ' ') break;
            print(Serial, "Waiting for power up, skip by space...\n");
        }
    }
    print(Serial, "Powered from port {}\n", output_port);
    if (!wifi::connect(SSID, PSWD))
        trap("Can't connect to wifi.");
    controller_ip = WiFi.gatewayIP();
    print(Serial, "My IP: {}\n", WiFi.localIP().toString().c_str());
    print(Serial, "Controller IP: {}\n", controller_ip.toString().c_str());
    server.begin(server_port, server_process);
    print(Serial, "Sent: {}\n", sendLine(controller_ip, server_port, CMD::flasher));
}

void loop() {
    server.process();
    if (Serial.available()) {
        char c = Serial.read();
        switch(c) {
        case '\r':
            Serial.write('\n');
            break;
        case 'i':
            print(Serial, "Sent: {}\n", sendLine(controller_ip, server_port, CMD::flasher));
            break;
        case '2':
            print(Serial, "Sent: {}\n", sendLine({192, 168, 4, 2}, server_port, CMD::flasher));
            break;
        case '3':
            print(Serial, "Sent: {}\n", sendLine({192, 168, 4, 3}, server_port, CMD::flasher));
            break;
        case 'R':
            ESP.restart();
            break;
        }
    }
}

} // namespace flasher
