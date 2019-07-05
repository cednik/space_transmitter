#include <thread>
#include <mutex>

namespace controller {

void setup() {
    display.backlight();
    display.cursor(display.OFF);
    display.enable_scrolling(false);
    print(display, "Space transmitter control unit\n");
    wait(sec(1));
    display.clear();
    display.cursor(display.ON);
}

void loop() {
    if (keyboard.available()) {
        display.write(char(keyboard.read()));
    }
}

} // namespace controller
