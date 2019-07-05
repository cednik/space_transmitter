#include <Arduino.h>

enum class DeviceType { CONTROLLER, FLASHER };
DeviceType device_type = DeviceType::FLASHER;

#include "hw_def.hpp"

#include <string>

using namespace std;

#include <format.h>
using fmt::print;

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
