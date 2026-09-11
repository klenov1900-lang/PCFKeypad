/*
  Array_Commands.ino
  
  Универсальный пример: ввод команд через клавиатуру
  для выбора значений из массивов.
  
  Формат команды: #X N#
    X = a, b, c, d (имя массива)
    N = 1-10 (индекс)
  
  Примеры:
    #a1#  -> arrayA[0]
    #b3#  -> arrayB[2]
    #d10# -> arrayD[9]
    #*#   -> очистка буфера
  
  Вывод — только в Serial Monitor (для универсальности).
  
  Подключение:
    Arduino A4 (SDA) -> PCF8574 SDA
    Arduino A5 (SCL) -> PCF8574 SCL
    Arduino 5V       -> PCF8574 VCC
    Arduino GND      -> PCF8574 GND
*/

#include <Wire.h>
#include "PCFKeypad.h"

// Стандартная клавиатура 4x4
PCFKeypad keypad(0x20);

// ============================================================
// МАССИВЫ ДАННЫХ (по 10 значений)
// ============================================================
int arrayA[10] = {100, 200, 300, 400, 500, 600, 700, 800, 900, 1000};
int arrayB[10] = {11, 22, 33, 44, 55, 66, 77, 88, 99, 110};
int arrayC[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int arrayD[10] = {15, 25, 35, 45, 55, 65, 75, 85, 95, 105};

// ============================================================
// ПЕРЕМЕННЫЕ
// ============================================================
#define CMD_BUFFER_SIZE 8
char cmdBuffer[CMD_BUFFER_SIZE];
uint8_t cmdIndex = 0;
unsigned long lastCmdPressTime = 0;
const unsigned long CMD_TIMEOUT = 10000;

// ============================================================
// ФУНКЦИИ РАБОТЫ С КОМАНДАМИ
// ============================================================
void clearCmdBuffer() {
    cmdIndex = 0;
    cmdBuffer[0] = '\0';
    Serial.println("[CMD] Buffer cleared");
}

void addToCmdBuffer(char key) {
    if (cmdIndex < CMD_BUFFER_SIZE - 1) {
        cmdBuffer[cmdIndex++] = key;
        cmdBuffer[cmdIndex] = '\0';
        lastCmdPressTime = millis();
        Serial.print("[CMD] ");
        Serial.println(cmdBuffer);
    }
}

void executeCommand() {
    // Проверка минимальной длины
    if (cmdIndex < 4) {
        Serial.println("[CMD] Too short");
        clearCmdBuffer();
        return;
    }
    
    char arrayName = cmdBuffer[1];
    uint8_t indexLen = cmdIndex - 3;
    
    // Проверка длины индекса
    if (indexLen < 1 || indexLen > 2) {
        Serial.println("[CMD] Invalid index length");
        clearCmdBuffer();
        return;
    }
    
    // Проверка, что индекс — цифры
    for (uint8_t i = 0; i < indexLen; i++) {
        if (cmdBuffer[2 + i] < '0' || cmdBuffer[2 + i] > '9') {
            Serial.println("[CMD] Index must be digits");
            clearCmdBuffer();
            return;
        }
    }
    
    // Преобразование индекса
    char indexStr[3] = {0};
    for (uint8_t i = 0; i < indexLen; i++) {
        indexStr[i] = cmdBuffer[2 + i];
    }
    int index = atoi(indexStr);
    
    // Проверка диапазона
    if (index < 1 || index > 10) {
        Serial.print("[CMD] Index out of range: ");
        Serial.println(index);
        clearCmdBuffer();
        return;
    }
    
    // Выбор массива
    uint8_t arrayIndex = index - 1;
    int result = 0;
    
    switch(arrayName) {
        case 'a': case 'A': result = arrayA[arrayIndex]; break;
        case 'b': case 'B': result = arrayB[arrayIndex]; break;
        case 'c': case 'C': result = arrayC[arrayIndex]; break;
        case 'd': case 'D': result = arrayD[arrayIndex]; break;
        default:
            Serial.println("[CMD] Unknown array");
            clearCmdBuffer();
            return;
    }
    
    Serial.print("[RESULT] ");
    Serial.print(cmdBuffer);
    Serial.print(" -> ");
    Serial.println(result);
    
    clearCmdBuffer();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
    Serial.begin(9600);
    Serial.println("=== PCFKeypad Array Commands ===");
    Serial.println("Commands: #X N#  where X=a,b,c,d  N=1-10");
    Serial.println("Examples:");
    Serial.println("  #a1#  -> arrayA[0] = 100");
    Serial.println("  #b3#  -> arrayB[2] = 33");
    Serial.println("  #d10# -> arrayD[9] = 105");
    Serial.println("  #*#   -> clear buffer");
    Serial.println("----------------------------------------");
    
    keypad.begin();
    keypad.setDebounceTime(50);
    keypad.setHoldDelay(500);
    keypad.setRepeatDelay(150);
    
    Serial.println("Ready!");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
    unsigned long currentTime = millis();
    
    // Таймаут ввода команды
    if (cmdIndex > 0 && (currentTime - lastCmdPressTime > CMD_TIMEOUT)) {
        Serial.println("[CMD] Timeout, clearing");
        clearCmdBuffer();
    }
    
    char state = keypad.getKey();
    char key = keypad.getCurrentKey();
    
    if (state == KEY_PRESSED) {
        if (key == '#') {
            if (cmdIndex == 0) {
                addToCmdBuffer(key);
            } else {
                addToCmdBuffer(key);
                executeCommand();
            }
        }
        else if (cmdIndex > 0) {
            if ((key >= '0' && key <= '9') || 
                (key >= 'a' && key <= 'd') || 
                (key >= 'A' && key <= 'D')) {
                addToCmdBuffer(key);
            }
            else if (key == '*') {
                clearCmdBuffer();
            }
            else {
                clearCmdBuffer();
            }
        }
        else {
            Serial.print("[KEY] ");
            Serial.println(key);
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
