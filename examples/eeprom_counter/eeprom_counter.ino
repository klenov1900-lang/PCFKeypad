/*
  EEPROM_Counter.ino
  
  Счётчик нажатий с сохранением в EEPROM.
  Значение счётчика сохраняется каждые 5 нажатий
  и восстанавливается при включении питания.
  
  Подключение:
    Arduino A4 (SDA) -> PCF8574 SDA
    Arduino A5 (SCL) -> PCF8574 SCL
    Arduino 5V       -> PCF8574 VCC
    Arduino GND      -> PCF8574 GND
*/

#include <Wire.h>
#include <EEPROM.h>
#include "PCFKeypad.h"

// Стандартная клавиатура 4x4
PCFKeypad keypad(0x20);

// Адрес в EEPROM для счётчика
#define EEPROM_ADDR 0

// Счётчик нажатий
int pressCounter = 0;

// ============================================================
// ФУНКЦИИ EEPROM
// ============================================================
void saveCounter(int value) {
    byte* p = (byte*)&value;
    for (int i = 0; i < sizeof(value); i++) {
        EEPROM.update(EEPROM_ADDR + i, p[i]);
    }
}

int loadCounter() {
    int value = 0;
    byte* p = (byte*)&value;
    for (int i = 0; i < sizeof(value); i++) {
        p[i] = EEPROM.read(EEPROM_ADDR + i);
    }
    return value;
}

bool isCounterValid() {
    int value = loadCounter();
    return (value >= 0 && value < 100000);
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(9600);
    Serial.println("=== PCFKeypad EEPROM Counter ===");
    
    keypad.begin();
    keypad.setDebounceTime(50);
    keypad.setHoldDelay(500);
    keypad.setRepeatDelay(150);
    
    // Загружаем счётчик из EEPROM
    if (isCounterValid()) {
        pressCounter = loadCounter();
        Serial.print("[EEPROM] Loaded: ");
        Serial.println(pressCounter);
    } else {
        pressCounter = 0;
        Serial.println("[EEPROM] No data, starting from 0");
    }
    
    Serial.println("Press any key...");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    char state = keypad.getKey();
    char key = keypad.getCurrentKey();
    
    if (state == KEY_PRESSED) {
        pressCounter++;
        Serial.print("[PRESS #");
        Serial.print(pressCounter);
        Serial.print("] ");
        Serial.println(key);
        
        // Сохраняем каждые 5 нажатий
        if (pressCounter % 5 == 0) {
            saveCounter(pressCounter);
            Serial.print("[EEPROM] Saved: ");
            Serial.println(pressCounter);
        }
    }
    else if (state == KEY_HOLD) {
        Serial.print("[HOLD] ");
        Serial.println(key);
    }
    else if (state == KEY_ERROR) {
        Serial.print("I2C Error: ");
        Serial.println(keypad.getLastError());
    }
    
    delay(10);
}
