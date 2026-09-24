/*
  PCFKeypad - Пример с прерываниями и энергосбережением
  
  Этот скетч демонстрирует работу клавиатуры в режиме низкого энергопотребления.
  Arduino "спит" (не сканирует матрицу) до тех пор, пока не будет нажата клавиша.
  
  Подключение:
  - PCF8574 INT пин -> D2 (Arduino Nano/Uno) или D3 (Mega)
  - Остальные пины I2C и клавиатуры подключаются стандартно
  
  Важно: Для работы этого примера необходим аппаратный пин INT на модуле PCF8574!
*/

#include <Wire.h>
#include "PCFKeypad.h"

// Адрес PCF8574 (0x20 для базового, 0x38 для AT без перемычек)
#define KEYPAD_ADDR 0x20 
// Пин прерывания (INT0 на Nano/Uno = D2)
#define INTERRUPT_PIN 2 

PCFKeypad keypad(KEYPAD_ADDR, INTERRUPT_PIN);

// Флаг для обработки событий вне ISR
volatile bool keyEventTriggered = false;

// Обработчик прерывания (должен быть максимально быстрым!)
void onKeypadInterrupt() {
  // Библиотечный метод атомарно устанавливает флаг _dataChanged
  keypad.handleInterrupt();
  // Устанавливаем наш пользовательский флаг для loop()
  keyEventTriggered = true;
}

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10); // Ждем Serial Monitor на некоторых платах
  
  Serial.println("=== Power Save & Interrupt Demo ===");
  
  // Инициализация клавиатуры
  keypad.begin();
  
  // Проверка связи перед стартом
  if (!keypad.isConnected()) {
    Serial.print("ERROR: PCF8574 not found! Code: ");
    Serial.println(keypad.getLastError());
    while(true) delay(1000);
  }
  Serial.println("PCF8574 connected OK");
  
  // Настройка параметров
  keypad.setDebounceTime(50);
  keypad.setHoldDelay(800);
  keypad.setRepeatDelay(200);
  
  // Включаем режим энергосбережения
  // Через 5 секунд бездействия библиотека перейдет в idle-режим
  keypad.setPowerSave(true);
  keypad.setPowerSaveTimeout(5000); 
  
  // Подключаем аппаратное прерывание
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), onKeypadInterrupt, FALLING);
  
  Serial.println("System ready. Press any key to wake up...");
  Serial.println("Device will enter power save after 5s of inactivity.");
}

void loop() {
  // Если было прерывание, обрабатываем событие
  if (keyEventTriggered) {
    keyEventTriggered = false;
    
    char state = keypad.getKey();
    char key = keypad.getCurrentKey();
    
    if (state == KEY_PRESSED) {
      Serial.print("[PRESSED] Key: ");
      Serial.println(key);
    } 
    else if (state == KEY_HOLD) {
      Serial.print("[HOLD]   Key: ");
      Serial.println(key);
    }
    else if (state == KEY_RELEASED) {
      // Можно логировать отпускание, если нужно
      // Serial.println("[RELEASED]");
    }
    else if (state == KEY_ERROR) {
      Serial.print("[ERROR] I2C code: ");
      Serial.println(keypad.getLastError());
    }
  }
  
  // Опционально: вывод статуса сна раз в секунду
  static uint32_t lastStatusPrint = 0;
  if (millis() - lastStatusPrint > 1000) {
    lastStatusPrint = millis();
    if (keypad.isPowerSaveMode()) {
      Serial.println(">> STATUS: POWER SAVE MODE ACTIVE <<");
    } else {
      Serial.println(">> STATUS: ACTIVE SCANNING >>");
    }
  }
  
  // Здесь может быть другой неблокирующий код вашего проекта
  // delay() НЕ нужен! Система работает полностью на событиях.
}
