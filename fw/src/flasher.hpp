#include <sstream>

#include "wifi.hpp"
#include "delayedTask.hpp"

namespace flasher {

using controller::controller_ip;
std::string ID;

int output_port = 0;

void set_led(uint8_t led, uint32_t brightness, DelayedTask::Time time = 0) {
    if (led == BUZZER)
        buzzer(brightness != 0);
    else
        ledcWrite(led, brightness);
    if (time != 0)
        DelayedTask::create(time, [led]() {
            if (led == BUZZER)
                buzzer(false);
            else
                ledcWrite(led, 0);
        } );
}

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
        if (arg.size() >= 2) {
            int brightness = 0;
            if ((std::istringstream(arg[1])>>brightness)) {
                DelayedTask::Time time = 0;
                if (arg.size() >= 3) {
                    if ((std::istringstream(arg[2])>>time)) {
                        if (arg.size() >= 4) {
                            SyncTask::create(arg[3],
                                [=](const std::string&, IPAddress, uint16_t) {
                                    set_led(led->second, brightness, time);
                                } );
                        } else {
                            set_led(led->second, brightness, time);
                        }
                    } else {
                        debug("Can not convert time to number");
                    }
                } else {
                    set_led(led->second, brightness, time);
                }
            } else {
                debug("Can not convert brightness to number");
            }
        } else {
            debug("Led without argument");
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
    SyncTask::begin(sync_port);
    sendLine(controller_ip, server_port, CMD::flasher);
}

void loop() {
    server.process();
    DelayedTask::process();
    SyncTask::process();
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
