#pragma once

#include <display.hpp>
#include <PS2Keyboard.h>
#include <SmartLeds.h>

#include <unordered_map>
#include <string>

const uint8_t PIN_GEN_P           = 36;
const uint8_t PIN_GEN_N           = 39;

const gpio_num_t PIN_DEV_TYPE     = GPIO_NUM_17;

const gpio_num_t I2C_SDA          = GPIO_NUM_0;
const gpio_num_t I2C_SCL          = GPIO_NUM_15;

const gpio_num_t PS2_DATA         = GPIO_NUM_23;
const gpio_num_t PS2_CLK          = GPIO_NUM_22;

const gpio_num_t PWR1_MEAS        = GPIO_NUM_34;
const gpio_num_t PWR2_MEAS        = GPIO_NUM_35;

const gpio_num_t PWR1_EN          = GPIO_NUM_27;
const gpio_num_t PWR2_EN          = GPIO_NUM_14;

const gpio_num_t PWR_EN          = GPIO_NUM_12;

const gpio_num_t IO1_EN           = GPIO_NUM_2;
const gpio_num_t IO_ILED_EN       = GPIO_NUM_18;
const gpio_num_t PIN_ILED         = GPIO_NUM_19;

const gpio_num_t PIN_RED          = GPIO_NUM_26;
const gpio_num_t PIN_GREEN        = GPIO_NUM_25;
const gpio_num_t PIN_BLUE         = GPIO_NUM_33;
const gpio_num_t PIN_WHITE        = GPIO_NUM_12;

const gpio_num_t PIN_BUZZER_P     = GPIO_NUM_13;
const gpio_num_t PIN_BUZZER_N     = GPIO_NUM_4;

const double PWM_FREQ = 20000; // Hz
const uint8_t PWM_RESOLUTION = 8; // bits

// PWM channel
enum LED_CHANNEL {
    RED    = 0,
    GREEN  = 1,
    BLUE   = 2,
    WHITE  = 3,
    BUZZER = 4
};

uint8_t BUZZER_ON = HIGH;
uint8_t BUZZER_OFF = LOW;

const int LED_COUNT = 1;
const int LED_CHANNEL = 0;

namespace detail {

typedef I2C_master I2C_internal;
I2C_internal i2c_internal ( I2C_NUM_0, I2C_SDA, I2C_SCL );

namespace display {

typedef PCF8574_t<I2C_internal> Display_port;

const int DISPLAY_WIDTH  = 20;
const int DISPLAY_HEIGHT =  4;
const int DISPLAY_PIN_E          = 2;
const int DISPLAY_PIN_RW         = 1;
const int DISPLAY_PIN_RS         = 0;
const int DISPLAY_PIN_DATA       = 4;
const int DISPLAY_PIN_BACKGLIGHT = 3;
const Display_port::address_type DISPLAY_ADDRESS = 0x3F;

Display_port display_port ( i2c_internal, DISPLAY_ADDRESS );

} // namespace display

} // namespace detail

typedef Display_t <
    detail::display::DISPLAY_WIDTH,
    detail::display::DISPLAY_HEIGHT,
    detail::display::DISPLAY_PIN_E,
    detail::display::DISPLAY_PIN_RW,
    detail::display::DISPLAY_PIN_RS,
    detail::display::DISPLAY_PIN_DATA,
    detail::display::DISPLAY_PIN_BACKGLIGHT > Display;
Display display ( detail::display::display_port );

PS2Keyboard keyboard;

SmartLed bargraf( LED_WS2812B, LED_COUNT, PIN_ILED, LED_CHANNEL, SingleBuffer );

typedef std::unordered_map<std::string, uint8_t> LED_MAP_t;
const LED_MAP_t LED( {
    { "r", RED    },
    { "g", GREEN  },
    { "b", BLUE   },
    { "w", WHITE  },
    { "a", BUZZER }
} );

void init_hw(void) {
    pinMode(PWR1_MEAS, INPUT);
    pinMode(PWR2_MEAS, INPUT);

    pinMode(PWR1_EN, OUTPUT);
    pinMode(PWR2_EN, OUTPUT);

    pinMode(IO1_EN, OUTPUT);
    digitalWrite(IO1_EN, HIGH);

    pinMode(IO_ILED_EN, OUTPUT);
    digitalWrite(IO_ILED_EN, HIGH);
    
    ledcSetup(RED, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(GREEN, PWM_FREQ, PWM_RESOLUTION);
    ledcSetup(BLUE, PWM_FREQ, PWM_RESOLUTION);
    
    ledcAttachPin(PIN_RED, RED);
    ledcAttachPin(PIN_GREEN, GREEN);
    ledcAttachPin(PIN_BLUE, BLUE);
    
    pinMode(PIN_BUZZER_N, OUTPUT);
    pinMode(PIN_BUZZER_P, OUTPUT);

    pinMode(PIN_DEV_TYPE, INPUT_PULLUP);
    wait(msec(1));
    if (digitalRead(PIN_DEV_TYPE) == LOW) {

        device_type = DeviceType::CONTROLLER;

        pinMode(PIN_DEV_TYPE, INPUT);
        pinMode(PWR_EN, OUTPUT);
        digitalWrite(PIN_BUZZER_N, HIGH);
        digitalWrite(PIN_BUZZER_P, HIGH);
        BUZZER_OFF = HIGH;
        BUZZER_ON = LOW;
        detail::i2c_internal.init();
        display.init();
        keyboard.begin(PS2_DATA, PS2_CLK);
        for (int i = 0; i != LED_COUNT; ++i)
            bargraf[i] = Rgb(0, 0, 0);
        bargraf.show();
        bargraf.wait();
    } else {

        device_type = DeviceType::FLASHER;

        ledcSetup(WHITE, PWM_FREQ, PWM_RESOLUTION);
        ledcAttachPin(PIN_WHITE, WHITE);
    }
}
