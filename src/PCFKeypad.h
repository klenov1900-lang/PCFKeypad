#ifndef PCF_KEYPAD_H
#define PCF_KEYPAD_H

#include <Arduino.h>
#include <Wire.h>

// ============================================================
// СОСТОЯНИЯ КЛАВИШ
// ============================================================
#define KEY_RELEASED 0
#define KEY_PRESSED  1
#define KEY_HOLD     2
#define KEY_ERROR    0xFE

// ============================================================
// КОДЫ ОШИБОК I2C
// ============================================================
#define I2C_OK          0
#define I2C_ERROR       1
#define I2C_NO_DEVICE   2

class PCFKeypad {
public:
    // ============================================================
    // КОНСТРУКТОРЫ
    // ============================================================
    // Стандартная клавиатура 4x4 (R1-R4=P4-P7, C1-C4=P0-P3)
    PCFKeypad(uint8_t addr);

    // Своя распиновка
    PCFKeypad(uint8_t addr, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]);

    // С прерыванием
    PCFKeypad(uint8_t addr, uint8_t intPin, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]);

    // ============================================================
    // ОСНОВНЫЕ МЕТОДЫ
    // ============================================================
    void begin();
    char getKey();
    void handleInterrupt();
    void reset();

    // ============================================================
    // НАСТРОЙКА
    // ============================================================
    void setDebounceTime(uint16_t ms);
    void setHoldDelay(uint16_t ms);
    void setRepeatDelay(uint16_t ms);
    void setScanInterval(uint16_t ms);
    void setActiveLow(bool activeLow);

    // ============================================================
    // ЭНЕРГОСБЕРЕЖЕНИЕ
    // ============================================================
    void setPowerSave(bool enable);
    void setPowerSaveTimeout(uint32_t timeout);
    bool isPowerSaveMode();

    // ============================================================
    // ДОПОЛНИТЕЛЬНЫЕ
    // ============================================================
    bool isKeyPressed();
    char getCurrentKey();
    uint8_t getLastError();
    bool isConnected();

private:
    // ============================================================
    // ПАРАМЕТРЫ
    // ============================================================
    uint8_t _addr;
    uint8_t _intPin;
    bool _useInterrupt;
    bool _activeLow;
    uint8_t _lastError;
    volatile uint8_t _dataChanged;

    // ============================================================
    // МАССИВЫ
    // ============================================================
    uint8_t _rows[4];
    uint8_t _cols[4];
    char _keys[4][4];

    // ============================================================
    // МАСКИ
    // ============================================================
    uint8_t _rowMasks[4];
    uint8_t _colMasks[4];
    uint8_t _idleMask;

    // ============================================================
    // СОСТОЯНИЯ
    // ============================================================
    volatile bool _keyPressed;
    volatile char _currentKey;
    bool _powerSaveMode;
    bool _powerSaveEnabled;
    uint32_t _powerSaveTimeout;
    uint32_t _lastActivityTime;

    // ============================================================
    // ВРЕМЕННЫЕ МЕТКИ (все в миллисекундах!)
    // ============================================================
    uint32_t _lastPressTime;
    uint32_t _lastRepeatTime;
    uint32_t _holdStartTime;
    uint32_t _lastScanTime;         // для _scanKey()
    uint32_t _lastAnyKeyScanTime;   // для _isAnyKeyPressed() — разделено!

    // ============================================================
    // НАСТРОЙКИ (все в миллисекундах!)
    // ============================================================
    uint16_t _debounceDelay;
    uint16_t _holdDelay;
    uint16_t _repeatDelay;
    uint16_t _scanInterval;
    uint16_t _scanIntervalIdle;
    uint32_t _lastIdleTime;

    // ============================================================
    // ПРИВАТНЫЕ МЕТОДЫ
    // ============================================================
    void _init(uint8_t addr, uint8_t intPin, bool useInterrupt,
               const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]);
    void _calculateMasks();
    void _writePCF(byte data);
    byte _readPCF();
    char _scanKey();
    bool _isAnyKeyPressed();
    bool _checkGhosting(uint8_t row, uint8_t col);
    uint8_t _i2cWrite(byte data);
    uint8_t _i2cRead(byte &data);
    void _enterPowerSave();
    void _exitPowerSave();
    bool _checkActivity();
    void _atomicSetChanged(bool value);
    bool _atomicGetChanged();
};

#endif