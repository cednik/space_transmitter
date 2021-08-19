#include <Arduino.h>
#include <Esp.h>

#include <string>
#include <vector>
#include <climits>
#include <sstream>

#include <format.h>
using fmt::print;
using fmt::format;
using namespace std;

#include "hw_def.hpp"
#include "adc.hpp"
#include "wifi.hpp"
#include "server.hpp"

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

void buzzer(bool en) { digitalWrite(PIN_BUZZER_P, en ? LOW : HIGH); }

std::vector<std::string> split(const std::string& str, char separator = ' ', size_t max = UINT_MAX) {
    std::vector<std::string> res;
    size_t pos = 0;
    for(;max != 0; --max) {
        size_t next = str.find(separator, pos);
        res.push_back(str.substr(pos, next-pos));
        while(str[next] == separator) {
            if (++next == str.size()) {
                next = std::string::npos;
                break;
            }
        }
        pos = next;
        if (pos == std::string::npos)
            break;
    }
    return res;
}

#include "delayedTask.hpp"
#include <screens.hpp>

ScrollLine<Display> status(display, 3, 0, 0, "", false);
ScrollLine<Display> status1(display, 2, 0, 0, "", false);
typedef TerminalScreen<Display, PS2Keyboard> Terminal;
void cmd_parser(std::string input, Terminal& terminal);
Terminal terminal(display, keyboard, ">>>", cmd_parser, 0, 0);

LineServer tcp_server;

timeout generator_meas(msec(50));
const int generator_print_div = 4;

Adc adc_gen_p(PIN_GEN_P);
Adc adc_gen_n(PIN_GEN_N);

int generator_print_prescaller = 0;
int charge = 0;
int pwr_sum = 0;

void cmd_parser(std::string input, Terminal& terminal) {
    static bool sudo = false;
    auto arg = split(input);
    if (arg.size() == 0)
        return;
    if (input == "sudo") {
        terminal.printLine("password:");
        terminal.onEnter([&](std::string input, Terminal& terminal){
            if (input == "lopaticka")
                sudo = true;
            else
                terminal.printLine("Access denied");
            terminal.onEnter(cmd_parser);
        });
    } else if (input == "help") {
        terminal.printLine("No help");
    } else if (sudo) {
        if (input == "on") {
            digitalWrite(PWR_EN, HIGH);
        } else if (input == "on1") {
            digitalWrite(PWR1_EN, HIGH);
        } else if (input == "on2") {
            digitalWrite(PWR2_EN, HIGH);
        } else if (input == "off") {
            digitalWrite(PWR_EN, LOW);
        } else if (input == "off1") {
            digitalWrite(PWR1_EN, LOW);
        } else if (input == "off2") {
            digitalWrite(PWR2_EN, LOW);
        } else if (input == "reboot") {
            ESP.restart();
        } else {
            terminal.printLine("Unknown cmd");
        }
    } else {
        terminal.printLine("Unknown cmd");
    }
}

uint8_t hue;
void showGradient() {
    hue++;
    // Use HSV to create nice gradient
    for ( int i = 0; i != LED_COUNT; i++ )
        leds[ i ] = Hsv{ static_cast< uint8_t >( hue + 30 * i ), 255, 255 };
    // Show is asynchronous; if we need to wait for the end of transmission,
    // we can use leds.wait(); however we use double buffered mode, so we
    // can start drawing right after showing.
    leds.wait();
    leds.show();
}

timeout pwr_off_timer(msec(2000));

const size_t BLOWRES = 3;
const gpio_num_t BLOW_EN[BLOWRES] = { PWR1_EN, PWR3_EN, PWR4_EN };
const gpio_num_t PWR_LED = PWR2_EN;

#if 1
const timeout::time_type blow_delay[BLOWRES] = {
    sec(  2 * 3600),
    sec(  1 *   60),
    sec( 10 *   60)
};
int blackbox_mode = 32768;
#else
const timeout::time_type blow_delay[BLOWRES] = {
    sec(  1 ),
    sec(  5 ),
    sec(  10 )
};
int blackbox_mode = 327;
#endif

timeout blow(blow_delay[0]);
timeout blow_stop(msec(1000));
int blow_index = 0;
int blow_stop_index = -1;

int disp_on = 50;
int led_on = 200;
//int blackbox_mode = 32768;
int blackbox_length = 32;
int blackbox_charge_step = 64;
int blackbox_brightness = 32;
int blackbox_selection_brightness = 128;
int blackbox_ack_brightness = 128;
timeout blackbox_ack(msec(5000));

timeout discharge(msec(100));

void button_callback(const bool& state) {
    if (state) {
        pwr_off_timer.restart();
    } else {
        pwr_off_timer.cancel();
        blow.restart();
    }
}

void remote_parser(string msg, WiFiClient& client) {

}

void setup() {
    Serial.begin(115200);

    init_hw();

    digitalWrite(PWR_EN, HIGH);
    digitalWrite(PWR_LED, HIGH);
    
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    display.clear();
    print(display, "Ready");
    delay(2000);
    // display.clear();
    // display.backlight(0);
    // display.off();
    Adc::begin();
    generator_meas.restart();
    button.register_callback(button_callback);
    pwr_off_timer.cancel();
    blow.cancel();
    blow_stop.cancel();
    //wifi::connect("TechnikaCamp", "tednenestiham");
    //tcp_server.begin(1234, remote_parser);
}

void loop() {
    if (generator_meas) {
        generator_meas.ack();
        int pwr = adc_gen_p.value() - adc_gen_n.value();
        charge += pwr;
        pwr_sum += pwr;
        if (++generator_print_prescaller == generator_print_div) {
            status = format("P{:4} C{:6}", pwr_sum / generator_print_div, charge);
            if (pwr_sum != 0) {
                blackbox_ack.restart();
            }
            pwr_sum = 0;
            generator_print_prescaller = 0;
        }
        if (charge > disp_on) {
            if (!display.is_on()) {
                display.on();
                display.backlight();
            }
        } else {
            if (display.is_on()) {
                display.off();
                display.backlight(false);
            }
        }
        if (charge > led_on) {
            digitalWrite(PWR_LED, HIGH);
            for (int i = 0; i != LED_COUNT; ++i) {
                if (charge < blackbox_mode) {
                    int brightness = clamp((charge - led_on) - (32 * i), 0, 255);
                    leds[i] = Rgb(brightness, brightness, brightness);
                } else {
                    Rgb c;
                    int b = (i / blackbox_length) == ((charge - blackbox_mode) / blackbox_charge_step % (LED_COUNT / blackbox_length)) ? blackbox_selection_brightness : blackbox_brightness;
                    if (i + blackbox_length < LED_COUNT) {
                        switch ((i / blackbox_length) % 4) {
                        case 0: c = Rgb(b, 0, 0); break;
                        case 1: c = Rgb(b, b, 0); break;
                        case 2: c = Rgb(b, 0, b); break;
                        case 3: c = Rgb(b, b, 0); break;
                        }
                    } else if (blackbox_ack) {
                        blackbox_ack.restart();
                        c = Rgb(blackbox_ack_brightness, blackbox_ack_brightness, blackbox_ack_brightness);
                        if (0 && !blow.running()) {
                            blow.restart();
                        }
                    } else {
                        c = Rgb(0, 0, 0);
                    }
                    leds[i] = c;
                }
            }
            leds.show();
        } else {
            digitalWrite(PWR_LED, LOW);
        }
        
    }
    if (discharge.running() && discharge) {
        discharge.ack();
        if (charge > 0)
            --charge;
        else if (charge < 0)
            ++charge;
    }
    if (pwr_off_timer.running() && pwr_off_timer) {
        digitalWrite(PWR_EN, LOW);
        print("Power off\n");
    }
    if (blow) {
        if (blow_stop_index == -1) {
            print("Blow {}\n", blow_index);
            digitalWrite(BLOW_EN[blow_index], HIGH);
            blow_stop.restart();
            blow_stop_index = blow_index;
            switch(++blow_index) {
            case 1:
            case 2:
                blow.restart();
                break;
            case 3:
                blow_index = 0;
                blow.cancel();
                break;
            }
            blow.set_timeout(blow_delay[blow_index]);
        }
    }
    if (blow_stop) {
        if (blow_stop_index != -1) {
            digitalWrite(BLOW_EN[blow_stop_index], LOW);
            print("Blowen {}\n", blow_stop_index);
            blow_stop.cancel();
            blow_stop_index = -1;
        }
    }
    tcp_server.process();
    button_t::process_all();
    Adc::process();
    status.process();
    status1.process();
    terminal.process();
    DelayedTask::process();
}
