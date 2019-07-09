#include <thread>
#include <mutex>

#include <screens.hpp>

#include "adc.hpp"

namespace controller {

ScrollLine<Display> status(display, 0, 0, 0, "", false);

typedef TerminalScreen<Display, PS2Keyboard> Terminal;

void cmd_parser(std::string input, Terminal& terminal);

Terminal terminal(display, keyboard, ">>>", cmd_parser, 0, 1);

timeout meas(msec(50));
timeout status_print(msec(250));

Adc adc_gen_p(PIN_GEN_P);
Adc adc_gen_n(PIN_GEN_N);

int charge = 0;
int pwr_sum = 0;

IPAddress controller_ip;

std::vector<IPAddress> flashers;

void server_process(std::string input, WiFiClient& client) {
    debug(format("recvd from {}: {}", client.remoteIP().toString().c_str(), show_whites(input)));
    if (input == CMD::flasher) {
        sendLine(client.remoteIP(), server_port, format( "{} {}", CMD::id, flashers.size()));
        terminal.printLine(format("Flasher {} registered at {}", flashers.size(), client.remoteIP().toString().c_str()));
        flashers.push_back(client.remoteIP());
    }
}

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
    } else if (input == "shutdown") {
        digitalWrite(PWR_EN, LOW);
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
        } else if (input == "clrflash") {
            flashers.clear();
        } else if (input == "flash") {
            for (const auto& ip: flashers)
                sendLine(ip, server_port, "w 16 100");
        } else if (input == "reboot") {
            ESP.restart();
        } else {
            terminal.printLine("Unknown cmd");
        }
    } else {
        terminal.printLine("Unknown cmd");
    }
}

void setup() {
    device_name = "STCU";
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
    server.begin(server_port, server_process);
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
    server.process();
    Adc::process();
    status.process();
    terminal.process();
}

} // namespace controller
