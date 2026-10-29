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

struct Rc5Frame {
    uint8_t address;
    uint8_t command;
    uint8_t toggle;
    uint16_t gapMs;  // Carrier-free gap before the next frame.
};

// https://www.majority.nl/tv_remote_control_codes.htm
// SERVICE TOOL -> ALIGN; toggles decoded from GFL "Alignment alternative".
// This sequence still needs validation on the Philips A8.0A-AA chassis.
constexpr Rc5Frame kAlignSequence[] = {
    {10, 26, 0, 15},
    {16, 7, 0, 20},
    {21, 92, 0, 225},  // Extended RC5; sendRC5 handles the command field bit.
    {31, 4, 0, 5},
    {31, 4, 1, 0},
};

void sendAlign() {
    digitalWrite(kTxLedPin, LOW);
    for (const auto& frame : kAlignSequence) {
        // The setter prepares an inverted value; sendRC5 must toggle it once.
        IrSender.setNextToggleBitValueForRC5AndRC6(frame.toggle);
        IrSender.sendRC5(frame.address, frame.command, 0, true);

        if (frame.gapMs != 0) {
            // Match the reference's last-mark-to-next-mark gaps. RC5 already
            // adds a leading space, plus a trailing space for an even command.
            const uint32_t trailingSpaceUs = (frame.command & 1) ? 0 : RC5_UNIT;
            const uint32_t gapUs = frame.gapMs * 1000UL;
            IrSender.customDelayMicroseconds(gapUs - RC5_UNIT - trailingSpaceUs);
        }
    }
    digitalWrite(kTxLedPin, HIGH);
    Serial.println("Sent ALIGN (5 RC5 frames; toggles=0,0,0,0,1).");
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
    Serial.println("M5StickC-RC7150: press Button A to send ALIGN.");
}

void loop() {
    static bool buttonAArmed = true;
    M5.update();
    if (M5.BtnA.isReleased()) {
        buttonAArmed = true;
    }
    if (buttonAArmed && M5.BtnA.wasPressed()) {
        buttonAArmed = false;
        // Blocking transmission ignores input until the whole sequence ends.
        sendAlign();
    }
    delay(5);
}
