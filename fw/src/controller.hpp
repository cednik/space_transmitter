#include <thread>
#include <mutex>

#include <screens.hpp>

namespace controller {

typedef TerminalScreen<Display, PS2Keyboard> Terminal;

void onEnter(std::string input, Terminal& terminal) {
    terminal.printLine("Got " + input);
}

Terminal terminal(display, keyboard, ">>>", onEnter);

void setup() {
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    print(display, "Space transmitter control unit\n");
    wait(sec(1));
    display.clear();
    terminal.show();
}

void loop() {
    terminal.process();
}

} // namespace controller
