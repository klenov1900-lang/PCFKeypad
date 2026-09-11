#include <Wire.h>
#include "PCFKeypad.h"

// Стандартная клавиатура 4x4 (всё по умолчанию)
PCFKeypad keypad(0x20);

void setup() {
    Serial.begin(9600);
    Serial.println("=== Keypad Test ===");
    
    keypad.begin();
    keypad.setDebounceTime(50);
    keypad.setHoldDelay(500);
    keypad.setRepeatDelay(150);
    
    Serial.println("Press any key...");
}

void loop() {
    char state = keypad.getKey();
    char key = keypad.getCurrentKey();
    
    if (state == KEY_PRESSED) {
        Serial.print("Key: ");
        Serial.println(key);
    }
    else if (state == KEY_HOLD) {
        Serial.print("Hold: ");
        Serial.println(key);
    }
    else if (state == KEY_ERROR) {
        Serial.println("I2C Error!");
    }
    
    delay(10);
}