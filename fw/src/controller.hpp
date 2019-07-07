#include <thread>
#include <mutex>

#include <screens.hpp>

#include "adc.hpp"

namespace controller {

ScrollLine<Display> status(display, 0, 0, 0, "", false);

typedef TerminalScreen<Display, PS2Keyboard> Terminal;

void onEnter(std::string input, Terminal& terminal) {
    terminal.printLine("Got " + input);
}

Terminal terminal(display, keyboard, ">>>", onEnter, 0, 1);

timeout generator_meas(msec(50));
const int generator_print_div = 4;

Adc adc_gen_p(PIN_GEN_P);
Adc adc_gen_n(PIN_GEN_N);

int generator_print_prescaller = 0;
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
    generator_meas.restart();
}

void loop() {
    if (generator_meas) {
        generator_meas.ack();
        int pwr = adc_gen_p.value() - adc_gen_n.value();
        charge += pwr;
        pwr_sum += pwr;
        if (++generator_print_prescaller == generator_print_div) {
            status = format("P{:4} C{:6}", pwr_sum / generator_print_div, charge);
            pwr_sum = 0;
            generator_print_prescaller = 0;
        }
    }
    Adc::process();
    status.process();
    terminal.process();
}

} // namespace controller
