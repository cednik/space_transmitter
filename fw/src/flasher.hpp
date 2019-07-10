namespace flasher {

int output_port = -1;

void set_led(uint8_t led, uint32_t brightness, DelayedTask::Time time = 0) {
    if (led == BUZZER)
        buzzer(brightness != 0);
    else if (led == PWR1_EN || led == PWR2_EN)
        digitalWrite(led, brightness == 0 ? LOW : HIGH);
    else
        ledcWrite(led, brightness);
    if (time != 0) {
        DelayedTask::create(time, [led]() {
            set_led(led, 0);
        } );
    }
}

void setup() {
    print(Serial, "Flasher\n");
    print(Serial, "Waiting for power up, skip by space...\n");
    for(;;) {
        if (digitalRead(PWR1_MEAS) == HIGH) {
            digitalWrite(PWR1_EN, HIGH);
            output_port = PWR2_EN;
            break;
        } else if (digitalRead(PWR2_MEAS) == HIGH) {
            digitalWrite(PWR2_EN, HIGH);
            output_port = PWR1_EN;
            break;
        }
        if (Serial.available()) {
            char c = Serial.read();
            if (c == ' ') break;
            print(Serial, "Waiting for power up, skip by space...\n");
        }
    }
    print(Serial, "Powered from port {}\n", output_port);
    uint32_t t = msec(100);
    set_led(BLUE, 32, t);
    DelayedTask::create(t, [](){ digitalWrite(output_port, HIGH); });
}

void loop() {
}

} // namespace flasher
