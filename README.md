# PCFKeypad

Библиотека для работы с клавиатурой 4×4 через I2C расширитель **PCF8574** на Arduino.

## ✨ Возможности

- ✅ **Сканирование матрицы 4×4** — все 16 клавиш
- ✅ **Антидребезг** — настраиваемый (по умолчанию 50 мс)
- ✅ **Автоповтор** — при удержании клавиши
- ✅ **Энергосбережение** — переход в сон через заданное время
- ✅ **Защита от ghosting** — игнорирование фантомных нажатий
- ✅ **Игнорирование множественного нажатия** — возвращается только "чистое" одиночное нажатие
- ✅ **Обработка ошибок I2C** — коды ошибок
- ✅ **Стандартная распиновка** — работает "из коробки"
- ✅ **Поддержка прерываний** — опционально
- ✅ **Минимальный код** — 3 строки для запуска

## 📦 Установка

### Способ 1: Через Arduino IDE

1. Скачайте библиотеку как ZIP-архив
2. В Arduino IDE: **Скетч → Подключить библиотеку → Добавить .ZIP библиотеку**
3. Выберите скачанный файл

### Способ 2: Вручную

1. Скачайте папку `PCFKeypad`
2. Поместите её в `Arduino/libraries/`
3. Перезапустите Arduino IDE

### Способ 3: Git

```bash
cd ~/Arduino/libraries/
git clone https://github.com/klenov1900-lang/PCFKeypad.git
```

## 🔌 Подключение

### Схема подключения

```
Arduino Nano    ->    PCF8574    ->    Клавиатура 4x4
------------------------------------------------------
A4 (SDA)        ->    SDA
A5 (SCL)        ->    SCL
5V              ->    VCC
GND             ->    GND

PCF8574         ->    Клавиатура
--------------------------------
P4              ->    R1 (строка 1)
P5              ->    R2 (строка 2)
P6              ->    R3 (строка 3)
P7              ->    R4 (строка 4)
P0              ->    C1 (столбец 1)
P1              ->    C2 (столбец 2)
P2              ->    C3 (столбец 3)
P3              ->    C4 (столбец 4)
```

### Распиновка шлейфа клавиатуры

```
Номер провода:  1    2    3    4    5    6    7    8
Пин:          R1   R2   R3   R4   C1   C2   C3   C4
```

### Адрес PCF8574

- По умолчанию: **0x20**
- Если перемычки A0-A2 замкнуты: адрес меняется
- Проверить адрес можно I2C-сканером

## 🚀 Быстрый старт

```cpp
#include <Wire.h>
#include "PCFKeypad.h"

// Стандартная клавиатура 4x4 (всё по умолчанию)
PCFKeypad keypad(0x20);

void setup() {
    Serial.begin(9600);
    keypad.begin();
    keypad.setDebounceTime(50);
    keypad.setHoldDelay(500);
    keypad.setRepeatDelay(150);
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
    
    delay(10);
}
```

## 📚 API

### Конструкторы

```cpp
// Стандартная клавиатура 4x4
PCFKeypad(uint8_t addr);

// Своя распиновка
PCFKeypad(uint8_t addr, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]);

// С прерыванием
PCFKeypad(uint8_t addr, uint8_t intPin, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]);
```

### Основные методы

| Метод | Описание | Возврат |
|-------|----------|---------|
| `begin()` | Инициализация | — |
| `getKey()` | Получить состояние клавиши | `KEY_PRESSED`, `KEY_HOLD`, `KEY_RELEASED`, `KEY_ERROR` |
| `getCurrentKey()` | Текущая нажатая клавиша | `char` |
| `isKeyPressed()` | Нажата ли любая клавиша | `bool` |
| `isConnected()` | Проверка подключения PCF8574 | `bool` |
| `getLastError()` | Код последней ошибки I2C | `uint8_t` |
| `reset()` | Сброс состояния | — |

### Настройка

| Метод | Описание | По умолчанию |
|-------|----------|--------------|
| `setDebounceTime(ms)` | Время антидребезга | 50 мс |
| `setHoldDelay(ms)` | Задержка до автоповтора | 800 мс |
| `setRepeatDelay(ms)` | Интервал автоповтора | 150 мс |
| `setScanInterval(ms)` | Интервал сканирования | 1 мс |
| `setActiveLow(bool)` | Полярность сигнала | `true` |

### Энергосбережение

| Метод | Описание |
|-------|----------|
| `setPowerSave(bool)` | Включить/выключить |
| `setPowerSaveTimeout(ms)` | Таймаут до сна |
| `isPowerSaveMode()` | Проверка режима |

### Прерывания

| Метод | Описание |
|-------|----------|
| `handleInterrupt()` | Вызывать из ISR |

## 📊 Константы

```cpp
// Состояния
#define KEY_RELEASED 0    // Клавиша не нажата
#define KEY_PRESSED  1    // Клавиша только что нажата
#define KEY_HOLD     2    // Клавиша удерживается (автоповтор)
#define KEY_ERROR    0xFE // Ошибка I2C

// Коды ошибок I2C
#define I2C_OK          0 // Нет ошибки
#define I2C_ERROR       1 // Ошибка передачи
#define I2C_NO_DEVICE   2 // Устройство не найдено
```

## 🔔 Прерывания

### Пример с прерыванием

```cpp
#include <Wire.h>
#include "PCFKeypad.h"

const uint8_t rowPins[4] = {4, 5, 6, 7};
const uint8_t colPins[4] = {0, 1, 2, 3};

char keys[4][4] = {
    {'D', '#', '0', '*'},
    {'C', '9', '8', '7'},
    {'B', '6', '5', '4'},
    {'A', '3', '2', '1'}
};

PCFKeypad keypad(0x20, 2, rowPins, colPins, keys);  // INT на D2

void IRAM_ATTR keypadISR() {
    keypad.handleInterrupt();
}

void setup() {
    Serial.begin(9600);
    keypad.begin();
    attachInterrupt(digitalPinToInterrupt(2), keypadISR, FALLING);
}

void loop() {
    char state = keypad.getKey();
    if (state == KEY_PRESSED) {
        Serial.println(keypad.getCurrentKey());
    }
    delay(10);
}
```

## 📁 Примеры

| Пример | Описание |
|--------|----------|
| `Basic` | Минимальный пример |
| `EEPROM_Counter` | Счётчик нажатий с сохранением в EEPROM |
| `With_Display` | С дисплеем TM1637 |

## ⚡ Производительность

| Параметр | Значение |
|----------|----------|
| Flash | ~7.5 КБ (24%) |
| RAM | ~841 байт (41%) |
| Скорость сканирования | 1 мс |
| Антидребезг | 50 мс |
| Автоповтор | 500/150 мс |

*Для Arduino Nano (ATmega328P)*

## 🐛 Обработка ошибок

```cpp
char state = keypad.getKey();

if (state == KEY_ERROR) {
    uint8_t error = keypad.getLastError();
    
    switch(error) {
        case I2C_NO_DEVICE:
            Serial.println("PCF8574 not found!");
            break;
        case I2C_ERROR:
            Serial.println("I2C communication error!");
            break;
    }
}
```

## 📋 Совместимость

| Платформа | Статус |
|-----------|--------|
| Arduino Nano | ✅ |
| Arduino Uno | ✅ |
| Arduino Mega | ✅ |
| ESP32 | ⚠️ (не тестировалось) |
| ESP8266 | ⚠️ (не тестировалось) |
| STM32 | ⚠️ (не тестировалось) |

## 📄 Лицензия

MIT License. См. [LICENSE](LICENSE).

## 📧 Контакты

- **Email:** gray_wolf19@mail.ru
- **GitHub:** [klenov1900-lang](https://github.com/klenov1900-lang)

---

⭐ Если библиотека вам полезна, поставьте звезду на GitHub! ⭐
