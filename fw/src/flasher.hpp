#include <sstream>

#include "wifi.hpp"
#include "delayedTask.hpp"

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
        DelayedTask::Time time = 0;
        switch (arg.size()) {
        case 1:
            debug(format("Led {} without argument!\n", cmd));
            break;
        case 2:
            if ((std::istringstream(arg[1])>>brightness))
                ledcWrite(led->second, brightness);
            else
                debug("Can not convert brightness to number");
            break;
        case 3:
        default:
            if (!(std::istringstream(arg[1])>>brightness)) {
                debug("Can not convert brightness to number");
                break;
            }
            if (!(std::istringstream(arg[2])>>time)) {
                debug("Can not convert time to number");
                break;
            }
            ledcWrite(led->second, brightness);
            DelayedTask::create(msec(time), [led]() {
                ledcWrite(led->second, 0);
                debug(format("Switch off led {}.", led->first));
            } );
            break;
        }
    } else if (cmd == CMD::power_next) {
        const uint8_t value = (arg.size() >= 2 && arg[1] == "0") ? LOW : HIGH;
        switch(output_port) {
            case 1: digitalWrite(PWR1_EN, value); break;
            case 2: digitalWrite(PWR2_EN, value); break;
        }
    } else if (cmd == CMD::id) {
        if (arg.size() >= 2) {
            ID = arg[1];
            debug(format("ID set to {}", ID));
        } else
            debug("ID without argument!");
    } else {
        debug(format("Unknown command \"{}\"\n", cmd));
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
    sendLine(controller_ip, server_port, CMD::flasher);
}

void loop() {
    server.process();
    DelayedTask::process();
    if (Serial.available()) {
        char c = Serial.read();
        switch(c) {
        case '\r':
            Serial.write('\n');
            break;
        case 'R':
            ESP.restart();
            break;
        }
    }
}

} // namespace flasher
