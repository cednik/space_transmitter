#include <screens.hpp>
#include <sstream>

namespace controller {

typedef TerminalScreen<Display, PS2Keyboard> Terminal;

void cmd_parser(std::string input, Terminal& terminal);

Terminal terminal(display, keyboard, ">>>", cmd_parser, 0, 0);

int settings_complete = 0;

timeout s(msec(1000));
uint32_t pwrup_time = 0;
const uint32_t shutdown_time = 1800;

void shutdown_process() {
    if (s) {
        s.ack();
        if (++pwrup_time >= shutdown_time)
            digitalWrite(PWR_EN, LOW);
    }
}

int tx_pin = -1;
int rx_pin = -1;

void cmd_parser(std::string input, Terminal& terminal) {
    pwrup_time = 0;
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
    } else if (input == "shutdown") {
        digitalWrite(PWR_EN, LOW);
    } else if (arg[0] == "antenna") {
        if (arg.size() != 2) {
            terminal.printLine("Bad arguments count");
        } else {
            if (arg[1] == "hexagon") {
                terminal.printLine("Antenna type set");
                settings_complete |= 1;
            } else {
                terminal.printLine("Unknown antenna type");
            }
        }
    } else if (arg[0] == "polarity") {
        if (arg.size() != 2) {
            terminal.printLine("Bad arguments count");
        } else {
            if (arg[1] == "circccw") {
                terminal.printLine("Polarity set");
                settings_complete |= 2;
                tx_pin = PWR1_EN;
                rx_pin = PWR2_MEAS;
            } else if (arg[1] == "circcw") {
                terminal.printLine("Polarity set");
                settings_complete |= 2;
                tx_pin = PWR2_EN;
                rx_pin = PWR1_MEAS;
            } else {
                terminal.printLine("Unknown polarity type");
            }
        }
    } else if (arg[0] == "freq") {
        if (arg.size() != 2) {
            terminal.printLine("Bad arguments count");
        } else {
            double freq = 0;
            if ((std::istringstream(arg[1]) >> freq)) {
                if (freq < 1800 || freq > 2000) {
                    terminal.printLine("Frequency out of range");
                } else {
                    terminal.printLine(format("Frequency set to {}", freq));
                    settings_complete |= 4;
                }
            } else {
                terminal.printLine("Not a number");
            }
        }
    } else if (arg[0] == "message") {
        if (settings_complete != 7) {
            terminal.printLine("Antenna not set");
        } else if (arg.size() != 1) {
            terminal.printLine("Bad arguments count");
        } else {
            terminal.m_input.prompt().set("...");
            terminal.m_input.prompt().process();
            terminal.onEnter([](std::string input, Terminal& terminal) {
                if (input.size() > 96) {
                    terminal.printLine("Too long message");
                } else {
                    terminal.printLine("Transmitting...");
                    digitalWrite(tx_pin, HIGH);
                    timeout t(sec(10));
                    while(digitalRead(rx_pin) == LOW) {
                        terminal.view_process();
                        shutdown_process();
                        if (t)
                            break;
                    }
                    terminal.printLine("Message sent");
                    terminal.printLine("Low energy - shutting down");
                    DelayedTask::create(sec(4), [](){ digitalWrite(PWR_EN, LOW); });
                }
                terminal.m_input.prompt().set(">>>");
                terminal.m_input.prompt().process();
                terminal.onEnter(cmd_parser);
            });
        }
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
        } else if (input == "set") {
            settings_complete = 7;
        } else {
            terminal.printLine("Unknown cmd");
        }
    } else {
        terminal.printLine("Unknown cmd");
    }
}

void setup() {
    digitalWrite(PWR_EN, HIGH);
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    display.clear();
    print(display, "Ready");
    delay(2000);
    display.clear();
    display.backlight(0);
    display.off();
    s.restart();
    // while (!(keyboard.available() && keyboard.read() == PS2_ENTER))
    // {
    //     delay(100);
    //     shutdown_process();
    // }
    // pwrup_time = 0;
    // display.backlight(1);
    // display.on();
    // terminal.show();
    // terminal.printLine("STCU v1.0 by kubas");
}

timeout blik(msec(1));
uint8_t hue;
void showGradient() {
    hue++;
    // Use HSV to create nice gradient
    for ( int i = 0; i != LED_COUNT; i++ )
        leds[ i ] = Hsv{ static_cast< uint8_t >( hue + 30 * i ), 255, 255 };
    // Show is asynchronous; if we need to wait for the end of transmission,
    // we can use leds.wait(); however we use double buffered mode, so we
    // can start drawing right after showing.
}
int _offset = 0;
int offset(int v) {
    int res = 1 + (v + _offset) % (LED_COUNT - 1);
    //print("v {:d}\to {:d}\tr {:d}\n", v, _offset, res);
    return res;
}
bool fallback = 0;
int mode = 0;
void loop() {
    terminal.process();
    shutdown_process();
    if (mode == 0 || blik) {
        switch (mode)
        case 0:
            blik.force();
            showGradient();
            break;
        case 1:
            leds[0] = ((_offset % 100) > 50 ) ? Rgb(64, 64, 64) : Rgb(0, 0, 0);
            for (int i = 1; i != LED_COUNT; ++i) {
                if (i % (LED_COUNT / 10) == 0)
                    leds[offset(i)] = Rgb(64, 64, 64);
                else
                    leds[offset(i)] = Rgb(0, 0, 0);
            }
            ++_offset;
            break;
        case 2:

        }
        leds.wait();
        leds.show();
        //print("blik\n");
    }
}

} // namespace controller
