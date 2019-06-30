#include <Arduino.h>

#include <string>
#include <thread>

using namespace std;

#include <format.h>
using fmt::print;

#define PWR1_MEAS 34
#define PWR2_MEAS 35

#define PWR1_EN 27
#define PWR2_EN 14

#define IO1_EN 2

#define PIN_RED      26
#define PIN_GREEN    25
#define PIN_BLUE     33
#define PIN_WHITE    12

#define PIN_BUZZER_P GPIO_NUM_13
#define PIN_BUZZER_N GPIO_NUM_4

#define PWM_FREQ 20000
#define PWM_RESOLUTION 8

#define RED     0
#define GREEN   1
#define BLUE    2
#define WHITE   3
#define BUZZER  4

void trap(const string& msg = "")
{
    print(Serial, "trap %\n", msg);
    while(1);
}

std::thread input_thread;
void input_loop();


void setup() {
    Serial.begin(115200);
    print(Serial, "Flasher\n");
    pinMode(PWR1_MEAS, INPUT);
    pinMode(PWR2_MEAS, INPUT);
    pinMode(PWR1_EN, OUTPUT);
    pinMode(PWR2_EN, OUTPUT);
    pinMode(IO1_EN, OUTPUT);
    digitalWrite(IO1_EN, HIGH);
    ledcSetup(RED, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(GREEN, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(BLUE, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(WHITE, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(PIN_RED, RED);
    ledcAttachPin(PIN_GREEN, GREEN);
    ledcAttachPin(PIN_BLUE, BLUE);
    ledcAttachPin(PIN_WHITE, WHITE);
    pinMode(PIN_BUZZER_N, OUTPUT);
    pinMode(PIN_BUZZER_P, OUTPUT);
    input_thread = std::thread(input_loop);
    ESP_LOGD(TAG, "Loop");
}

uint8_t color = 0;

void loop() {
    ledcWrite(RED  , color & 1  ? 16 : 0);
    ledcWrite(GREEN, color & 2  ? 16 : 0);
    ledcWrite(BLUE , color & 4  ? 16 : 0);
    ledcWrite(WHITE, color == 0 ? 16 : 0);
    digitalWrite(PWR2_EN, color > 7 ? HIGH : LOW);
    digitalWrite(PWR1_EN, color < 7 ? HIGH : LOW);
    digitalWrite(PIN_BUZZER_P, color == 8 ? HIGH : LOW);
    if (++color == 16)
        color = 0;
    delay(1000);
}

void input_loop() {
    bool pwr1 = false;
    bool pwr2 = false;
    Serial.println("input loop");
    while(true) {
        bool pwr = digitalRead(PWR1_MEAS);
        if (pwr != pwr1) {
            pwr1 = pwr;
            print(Serial, "pwr1 changed: {}\n", pwr ? "on" : "off");
        }
        pwr = digitalRead(PWR2_MEAS);
        if (pwr != pwr2) {
            pwr2 = pwr;
            print(Serial, "pwr2 changed: {}\n", pwr ? "on" : "off");
        }
        delay(10);
    }
}