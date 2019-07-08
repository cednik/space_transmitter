#include <Arduino.h>

enum class DeviceType { CONTROLLER, FLASHER };
DeviceType device_type = DeviceType::FLASHER;

#include "hw_def.hpp"
#include "server.hpp"

static const char* SSID = "STIC";
static const char* PSWD = "1123581321";
static const uint16_t server_port = 16384;

LineServer server;

#include <string>

using namespace std;

#include <format.h>
using fmt::print;
using fmt::format;

void trap(const string& msg = "")
{
    print(Serial, "trap %\n", msg);
    while(1);
}

void buzzer(bool en) { digitalWrite(PIN_BUZZER_P, en ? BUZZER_ON : BUZZER_OFF); }

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
}

void loop() {
    switch (device_type) {
        case DeviceType::CONTROLLER: controller::loop(); break;
        case DeviceType::FLASHER   :    flasher::loop(); break;
    }
}
