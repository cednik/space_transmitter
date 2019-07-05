#include <Arduino.h>

#include "hw_def.hpp"

#include <string>
#include <thread>
#include <mutex>

using namespace std;

#include <format.h>
using fmt::print;

void trap(const string& msg = "")
{
    print(Serial, "trap %\n", msg);
    while(1);
}

std::thread input_thread;
void input_loop();

std::thread output_thread;
void output_loop();

mutex disp_mutex;

void setup() {
    wait(sec(1));
    Serial.begin(115200);
    init_hw();
    display.backlight();
    //display.cursor(display.OFF);
    display.enable_scrolling(false);
    print(display, "Space transmitter control unit");
    print(Serial, "Flasher\n");
    wait(msec(100));
    input_thread = std::thread(input_loop);
    output_thread = std::thread(output_loop);
}

uint8_t color = 0;

void loop() {
    if (Serial.available()) {
        char c = Serial.read();
        switch(c) {
            case '\r':
                Serial.write('\n');
                break;
            case 'I':
                disp_mutex.lock();
                display.init();
                disp_mutex.unlock();
                print(display, "Disp init\n");
                break;
            case 'l':
                disp_mutex.lock();
                display.backlight(true);
                disp_mutex.unlock();
                break;
            case 'L':
                disp_mutex.lock();
                display.backlight(false);
                disp_mutex.unlock();
                break;
            case 'o':
                disp_mutex.lock();
                display.on(Display::ON);
                disp_mutex.unlock();
                break;
            case 'O':
                disp_mutex.lock();
                display.on(Display::OFF);
                disp_mutex.unlock();
                break;
            default:
                disp_mutex.lock();
                display.write(c);
                disp_mutex.unlock();
        }
    }
}

void output_loop() {
    while(true) {
        disp_mutex.lock();
        display.move_to(0, 1);
        print(display, "{}", color);
        disp_mutex.unlock();
        ledcWrite(RED  , color & 1  ? 16 : 0);
        ledcWrite(GREEN, color & 2  ? 16 : 0);
        ledcWrite(BLUE , color & 4  ? 16 : 0);
        ledcWrite(WHITE, color == 0 ? 16 : 0);
        digitalWrite(PWR2_EN, color > 7 ? HIGH : LOW);
        digitalWrite(PWR1_EN, color < 7 ? HIGH : LOW);
        digitalWrite(PIN_BUZZER_P, color == 8 ? BUZZER_ON : BUZZER_OFF);
        if (++color == 16)
            color = 0;
        delay(1000);
    }
}

void input_loop() {
    bool pwr1 = false;
    bool pwr2 = false;
    //Serial.println("input loop");
    while(true) {
        bool pwr = digitalRead(PWR1_MEAS);
        if (pwr != pwr1) {
            pwr1 = pwr;
            //print(Serial, "pwr1 changed: {}\n", pwr ? "on" : "off");
        }
        pwr = digitalRead(PWR2_MEAS);
        if (pwr != pwr2) {
            pwr2 = pwr;
            //print(Serial, "pwr2 changed: {}\n", pwr ? "on" : "off");
        }
        delay(10);
    }
}