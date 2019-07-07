#include <thread>
#include <mutex>

#include <screens.hpp>

#include "adc.hpp"

namespace controller {

ScrollLine<Display> status(display, 0, 0, 0, "", false);

typedef TerminalScreen<Display, PS2Keyboard> Terminal;

void cmd_parser(std::string input, Terminal& terminal) {
    static bool sudo = true;
    if (input == "sudo") {
        terminal.printLine("password:");
        terminal.onEnter([&](std::string input, Terminal& terminal){
            if (input == "lopaticka")
                sudo = true;
            else
                terminal.printLine("Access denied");
            terminal.onEnter(cmd_parser);
        });
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
        } else {
            terminal.printLine("Unknown cmd");
        }
    } else {
        terminal.printLine("Unknown cmd");
    }
}

Terminal terminal(display, keyboard, ">>>", cmd_parser, 0, 1);

timeout meas(msec(50));
timeout status_print(msec(250));

Adc adc_gen_p(PIN_GEN_P);
Adc adc_gen_n(PIN_GEN_N);

int charge = 0;
int pwr_sum = 0;

void setup() {
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    print(display, "Space transmitter control unit\n");
    wait(sec(1));
    display.clear();
    terminal.show();
    Adc::begin();
    meas.restart();
    status_print.restart();
}

void loop() {
    if (meas) {
        meas.ack();
        int pwr = adc_gen_p.value() - adc_gen_n.value();
        charge += pwr;
        pwr_sum += pwr;
    }
    if (status_print) {
        status_print.ack();
        status = format("P{:4} C{:6} {}{}",
            pwr_sum/5,
            charge,
            digitalRead(PWR2_MEAS),
            digitalRead(PWR1_MEAS));
        pwr_sum = 0;
    }
    Adc::process();
    status.process();
    terminal.process();
}

} // namespace controller
