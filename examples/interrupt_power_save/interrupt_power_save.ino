/*
  PCFKeypad - Пример: Прерывания и Энергосбережение
  
  Демонстрирует работу клавиатуры в режиме низкого энергопотребления.
  Arduino "спит" (не сканирует матрицу в цикле), пока не сработает прерывание от PCF8574.
  
  Подключение:
  - PCF8574 INT -> D2 (Arduino Nano/Uno)
  - PCF8574 SDA/SCL -> A4/A5
*/

#include <Wire.h>
#include "PCFKeypad.h"

#define KEYPAD_ADDR     0x20
#define INTERRUPT_PIN   2  // D2 на Nano/Uno

// ВАЖНО: Используем стандартные массивы, так как конструктор с прерыванием 
// требует передачи распиновки (согласно PCFKeypad.h)
const uint8_t rows[4] = {4, 5, 6, 7};
const uint8_t cols[4] = {0, 1, 2, 3};
char keys[4][4] = {
  {'D', '#', '0', '*'},
  {'C', '9', '8', '7'},
  {'B', '6', '5', '4'},
  {'A', '3', '2', '1'}
};

PCFKeypad keypad(KEYPAD_ADDR, INTERRUPT_PIN, rows, cols, keys);

volatile bool keyEventTriggered = false;

// Обработчик прерывания
void IRAM_ATTR onKeypadISR() {
  // Вызываем метод библиотеки (атомарная операция внутри)
  keypad.handleInterrupt();
  keyEventTriggered = true;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10);
  
  Serial.println("=== Power Save & Interrupt Demo ===");
  
  keypad.begin();
  
  if (!keypad.isConnected()) {
    Serial.print("ERROR: PCF8574 not found! Code: ");
    Serial.println(keypad.getLastError());
    while(true);
  }
  
  // Настройки
  keypad.setDebounceTime(50);
  keypad.setHoldDelay(800);
  
  // Включаем Power Save (таймаут 5 сек)
  keypad.setPowerSave(true);
  keypad.setPowerSaveTimeout(5000); 
  
  // Подключаем прерывание
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), onKeypadISR, FALLING);
  
  Serial.println("Ready. Press any key to wake up...");
}

void loop() {
  // Обработка события только если был флаг от ISR
  if (keyEventTriggered) {
    keyEventTriggered = false;
    
    char state = keypad.getKey();
    
    if (state == KEY_PRESSED) {
      Serial.print("[PRESSED] ");
      Serial.println(keypad.getCurrentKey());
    } 
    else if (state == KEY_HOLD) {
      Serial.print("[HOLD]    ");
      Serial.println(keypad.getCurrentKey());
    }
  }
  
  // Статус сна (для наглядности)
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    if (keypad.isPowerSaveMode()) {
      Serial.println(">> STATUS: SLEEPING (Power Save) <<");
    }
  }
  
  // Здесь нет delay() — цикл свободен для других задач
}
