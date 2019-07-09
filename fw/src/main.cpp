#include <Arduino.h>
#include <Esp.h>

#include <format.h>
using fmt::print;
using fmt::format;

#include "debug.hpp"

enum class DeviceType { CONTROLLER, FLASHER };
DeviceType device_type = DeviceType::FLASHER;

#include "hw_def.hpp"

#include "server.hpp"

static const char SSID[] = "STIC";
static const char PSWD[] = "1123581321";
static const uint16_t server_port = 16384;

LineServer server;

#include <string>
#include <vector>
#include <climits>

using namespace std;

void trap(const string& msg = "")
{
    print(Serial, "TRAP: {}\n", msg);
    for(;;) {
        ledcWrite(RED, 32);
        delay(20);
        ledcWrite(RED, 0);
        delay(180);
    }
}

void buzzer(bool en) { digitalWrite(PIN_BUZZER_P, en ? BUZZER_ON : BUZZER_OFF); }

std::vector<std::string> split(const std::string& str, char separator = ' ', size_t max = UINT_MAX) {
    std::vector<std::string> res;
    size_t pos = 0;
    for(;max != 0; --max) {
        size_t next = str.find(separator, pos);
        res.push_back(str.substr(pos, next-pos));
        while(str[next] == separator)
            ++next;
        pos = next;
        if (pos == std::string::npos)
            break;
    }
    return res;
}

#include "cmd.hpp"
#include "controller.hpp"
#include "flasher.hpp"

void setup() {
    wait(sec(1));
    Serial.begin(115200);
    init_hw();
    switch (device_type) {
        case DeviceType::CONTROLLER: controller::setup(); break;
        case DeviceType::FLASHER   :    flasher::setup(); break;
    }
    debug(device_name + " started\n");
}

void loop() {
    switch (device_type) {
        case DeviceType::CONTROLLER: controller::loop(); break;
        case DeviceType::FLASHER   :    flasher::loop(); break;
    }
}
