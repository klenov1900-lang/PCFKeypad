/*
  With_Display.ino
  
  Пример с 6-разрядным дисплеем TM1637.
  Требует библиотеку TM1637_6Easy.
  
  Подключение:
    TM1637:  CLK -> D8, DIO -> D9
    PCF8574: SDA -> A4, SCL -> A5
*/

#include <Wire.h>
#include "TM1637_6Easy.h"
#include "PCFKeypad.h"

// Дисплей TM1637 (CLK=8, DIO=9, яркость=3)
TM1637_6Easy display(8, 9, 3);

// Стандартная клавиатура 4x4
PCFKeypad keypad(0x20);

// ============================================================
// ТАБЛИЦА СЕГМЕНТНЫХ КОДОВ
// ============================================================
uint8_t charToIndex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    
    switch(c) {
        case 'A': case 'a': return 10;
        case 'B': case 'b': return 11;
        case 'C': case 'c': return 12;
        case 'D': case 'd': return 13;
        case '#': return 33;      // # -> =
        case '*': return 17;      // * -> ^
        default: return 20;       // пробел
    }
}

// ============================================================
// ФУНКЦИИ ОТОБРАЖЕНИЯ
// ============================================================
void displayKey(char key) {
    display.clear();
    
    if (key == 0) {
        display.showChars(16, 16, 20, 20, 20, 20);
        display.update();
        return;
    }
    
    display.showChar(2, charToIndex(key));
    display.update();
}

void displayKeyWithCounter(char key, int counter) {
    display.clear();
    
    if (key == 0) {
        display.showChars(16, 16, 20, 20, 20, 20);
        display.update();
        return;
    }
    
    display.showChar(2, charToIndex(key));
    
    if (counter > 0 && counter <= 99) {
        if (counter / 10 > 0) display.showChar(4, counter / 10);
        display.showChar(5, counter % 10);
    }
    
    display.update();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(9600);
    Serial.println("=== PCFKeypad + TM1637 ===");
    
    display.begin();
    display.setBrightness(7);
    display.clear();
    display.update();
    
    keypad.begin();
    keypad.setDebounceTime(50);
    keypad.setHoldDelay(500);
    keypad.setRepeatDelay(150);
    
    display.showString("HI");
    display.update();
    delay(1000);
    displayKey(0);
    
    Serial.println("Press any key...");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    static int pressCounter = 0;
    static bool wasPressed = false;
    static unsigned long lastKeyPressTime = 0;
    static bool displayShowKey = false;
    
    const unsigned long DISPLAY_SHOW_TIME = 1500;
    unsigned long currentTime = millis();
    
    char state = keypad.getKey();
    char key = keypad.getCurrentKey();
    bool isPressed = keypad.isKeyPressed();
    
    if (state == KEY_PRESSED) {
        pressCounter++;
        Serial.print("[PRESS #");
        Serial.print(pressCounter);
        Serial.print("] ");
        Serial.println(key);
        displayKeyWithCounter(key, pressCounter);
        displayShowKey = true;
        lastKeyPressTime = currentTime;
        wasPressed = true;
    }
    else if (state == KEY_HOLD) {
        Serial.print("[HOLD] ");
        Serial.println(key);
        displayKeyWithCounter(key, pressCounter);
        displayShowKey = true;
        lastKeyPressTime = currentTime;
    }
    else if (state == KEY_ERROR) {
        Serial.print("I2C Error: ");
        Serial.println(keypad.getLastError());
        display.showString("ERR");
        display.update();
        delay(1000);
        displayKey(0);
    }
    
    // Автоматическое скрытие
    if (displayShowKey && !isPressed) {
        if (currentTime - lastKeyPressTime > DISPLAY_SHOW_TIME) {
            displayKey(0);
            displayShowKey = false;
        }
    }
    
    delay(10);
}
