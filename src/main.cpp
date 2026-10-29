#include <Arduino.h>
#include <M5Unified.h>

// StickC Plus SE: the onboard IR driver is active-low on GPIO9.
// These options must precede IRremote.hpp.
#define IR_SEND_PIN 9
#define USE_ACTIVE_LOW_OUTPUT_FOR_SEND_PIN
#define DISABLE_CODE_FOR_RECEIVER
#define NO_LED_SEND_FEEDBACK_CODE
#include <IRremote.hpp>

namespace {
constexpr uint8_t kTxLedPin = 10;  // Active-low red status LED.
constexpr uint8_t kTvAddress = 0;
constexpr uint8_t kOsdCommand = 15;  // Majority.nl: SERVICE TOOL -> OSD.

void sendOsd() {
    digitalWrite(kTxLedPin, LOW);
    // One frame; change the RC5 toggle bit for each new button press.
    IrSender.sendRC5(kTvAddress, kOsdCommand, 0, true);
    digitalWrite(kTxLedPin, HIGH);
    Serial.println("Sent OSD (RC5 address=0 command=15)");
}
}  // namespace

void setup() {
    // Set inactive levels before enabling outputs, including during M5 startup.
    digitalWrite(IR_SEND_PIN, HIGH);
    pinMode(IR_SEND_PIN, OUTPUT);
    digitalWrite(kTxLedPin, HIGH);
    pinMode(kTxLedPin, OUTPUT);

    auto config = M5.config();
    config.serial_baudrate = 115200;
    config.internal_imu = false;  // The Plus SE has no IMU.
    config.internal_rtc = false;
    config.internal_mic = false;
    config.internal_spk = false;
    config.output_power = false;
    M5.begin(config);
    M5.Display.setBrightness(0);  // No on-device interface in this PoC.
    M5.BtnA.setDebounceThresh(25);

    IrSender.begin();
    Serial.println("M5StickC-RC7150: press Button A to send OSD.");
}

void loop() {
    M5.update();
    if (M5.BtnA.wasPressed()) {
        sendOsd();
    }
    delay(5);
}
