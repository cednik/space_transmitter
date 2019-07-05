#include <thread>
#include <mutex>

namespace controller {

void setup() {
    display.backlight();
    //display.cursor(display.OFF);
    display.enable_scrolling(false);
    print(display, "Space transmitter control unit");
    print(Serial, "Flasher\n");
}

void loop() {
    
}

} // namespace controller
