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

IPAddress controller_ip;

void registration_process(std::string msg, WiFiClient& client) {
    terminal.printLine(format("{}: {}", client.remoteIP().toString().c_str(), msg));
    client.print(format("Received \"{}\" from {}\n", msg, client.remoteIP().toString().c_str()).c_str());
}

LineServer registration_server(registration_server_port, registration_process);

void setup() {
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    display.clear();
    terminal.show();
    Adc::begin();
    terminal.printLine("STCU v1.0 by kubas");
    WiFi.softAP(SSID, PSWD);
    controller_ip = WiFi.softAPIP();
    terminal.printLine(format("CTRL IP {}", controller_ip.toString().c_str()));
    registration_server.begin();
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
    registration_server.process();
    Adc::process();
    status.process();
    terminal.process();
}

} // namespace controller
