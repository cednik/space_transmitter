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
        int c = keyboard.read();
        switch(c) {
        case 'r':
            bargraf[0] = Rgb(255, 0, 0);
            bargraf.show();
            break;
        case 'g':
            bargraf[0] = Rgb(0, 255, 0);
            bargraf.show();
            break;
        case 'b':
            bargraf[0] = Rgb(0, 0, 255);
            bargraf.show();
            break;
        default:
            display.write(char(c));
        }
    }
}

} // namespace controller
