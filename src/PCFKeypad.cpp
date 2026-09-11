#include "PCFKeypad.h"

// ============================================================
// КОНСТРУКТОР СО СТАНДАРТНОЙ КЛАВИАТУРОЙ
// ============================================================
PCFKeypad::PCFKeypad(uint8_t addr) {
    // Стандартная распиновка китайской клавиатуры 4x4
    // R1-R4 = P4-P7, C1-C4 = P0-P3
    const uint8_t defaultRows[4] = {4, 5, 6, 7};
    const uint8_t defaultCols[4] = {0, 1, 2, 3};
    char defaultKeys[4][4] = {
        {'D', '#', '0', '*'},
        {'C', '9', '8', '7'},
        {'B', '6', '5', '4'},
        {'A', '3', '2', '1'}
    };

    _init(addr, 0xFF, false, defaultRows, defaultCols, defaultKeys);
}

// ============================================================
// КОНСТРУКТОР С СВОЕЙ РАСПИНОВКОЙ
// ============================================================
PCFKeypad::PCFKeypad(uint8_t addr, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]) {
    _init(addr, 0xFF, false, rows, cols, keys);
}

// ============================================================
// КОНСТРУКТОР С ПРЕРЫВАНИЕМ
// ============================================================
PCFKeypad::PCFKeypad(uint8_t addr, uint8_t intPin, const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]) {
    _init(addr, intPin, true, rows, cols, keys);
}

// ============================================================
// ОБЩАЯ ИНИЦИАЛИЗАЦИЯ
// ============================================================
void PCFKeypad::_init(uint8_t addr, uint8_t intPin, bool useInterrupt,
                       const uint8_t rows[4], const uint8_t cols[4], char keys[4][4]) {
    _addr = addr;
    _intPin = intPin;
    _useInterrupt = useInterrupt;
    _activeLow = true;
    _lastError = I2C_OK;

    // Интервалы в миллисекундах
    _scanInterval = 1;          // 1 мс — активное сканирование
    _scanIntervalIdle = 5;      // 5 мс — в power save

    _powerSaveMode = false;
    _powerSaveEnabled = false;
    _powerSaveTimeout = 5000;
    _lastActivityTime = 0;
    _lastIdleTime = 0;
    _dataChanged = 0;

    // sizeof вместо магических чисел
    memcpy(_rows, rows, sizeof(_rows));
    memcpy(_cols, cols, sizeof(_cols));
    memcpy(_keys, keys, sizeof(_keys));

    _keyPressed = false;
    _currentKey = 0;
    _lastPressTime = 0;
    _lastRepeatTime = 0;
    _holdStartTime = 0;
    _lastScanTime = 0;
    _lastAnyKeyScanTime = 0;

    _debounceDelay = 50;
    _holdDelay = 800;
    _repeatDelay = 150;

    _calculateMasks();
}

// ============================================================
// АТОМАРНЫЕ ОПЕРАЦИИ
// ============================================================
void PCFKeypad::_atomicSetChanged(bool value) {
    noInterrupts();
    _dataChanged = value ? 1 : 0;
    interrupts();
}

bool PCFKeypad::_atomicGetChanged() {
    noInterrupts();
    bool result = (_dataChanged != 0);
    interrupts();
    return result;
}

// ============================================================
// РАСЧЁТ МАСОК
// ============================================================
void PCFKeypad::_calculateMasks() {
    if (_activeLow) {
        _idleMask = 0xFF;
        for (int i = 0; i < 4; i++) {
            _rowMasks[i] = 0xFF & ~(1 << _rows[i]);
            _colMasks[i] = 1 << _cols[i];
        }
    } else {
        _idleMask = 0x00;
        for (int i = 0; i < 4; i++) {
            _rowMasks[i] = 1 << _rows[i];
            _colMasks[i] = 0xFF & ~(1 << _cols[i]);
        }
    }
}

// ============================================================
// BEGIN
// ============================================================
void PCFKeypad::begin() {
    Wire.begin();
    Wire.setClock(100000);

    if (_useInterrupt && _intPin != 0xFF) {
        pinMode(_intPin, INPUT_PULLUP);
    }

    _writePCF(_idleMask);
    _lastError = I2C_OK;
    _powerSaveMode = false;
    _lastActivityTime = millis();
    _lastScanTime = 0;
    _lastAnyKeyScanTime = 0;
}

// ============================================================
// RESET
// ============================================================
void PCFKeypad::reset() {
    noInterrupts();
    _dataChanged = 0;
    _keyPressed = false;
    _currentKey = 0;
    interrupts();

    _lastPressTime = 0;
    _lastRepeatTime = 0;
    _holdStartTime = 0;
    _lastActivityTime = millis();
    _lastIdleTime = 0;
    _lastScanTime = 0;
    _lastAnyKeyScanTime = 0;
    _powerSaveMode = false;
    _writePCF(_idleMask);
}

// ============================================================
// ОБРАБОТКА ПРЕРЫВАНИЯ
// ============================================================
void PCFKeypad::handleInterrupt() {
    // Одна запись volatile — атомарна на AVR/ESP
    _dataChanged = 1;
}

// ============================================================
// I2C ОПЕРАЦИИ
// ============================================================
uint8_t PCFKeypad::_i2cWrite(byte data) {
    Wire.beginTransmission(_addr);
    Wire.write(data);
    uint8_t result = Wire.endTransmission();

    if (result != 0) {
        _lastError = (result == 2) ? I2C_NO_DEVICE : I2C_ERROR;
    } else {
        _lastError = I2C_OK;
    }
    return result;
}

uint8_t PCFKeypad::_i2cRead(byte &data) {
    Wire.requestFrom(_addr, (byte)1);
    if (Wire.available()) {
        data = Wire.read();
        _lastError = I2C_OK;
        return I2C_OK;
    }
    _lastError = I2C_ERROR;
    return I2C_ERROR;
}

void PCFKeypad::_writePCF(byte data) {
    _i2cWrite(data);
}

byte PCFKeypad::_readPCF() {
    byte data = _idleMask;
    _i2cRead(data);
    return data;
}

bool PCFKeypad::isConnected() {
    Wire.beginTransmission(_addr);
    uint8_t result = Wire.endTransmission();
    _lastError = (result == 0) ? I2C_OK : ((result == 2) ? I2C_NO_DEVICE : I2C_ERROR);
    return (result == 0);
}

// ============================================================
// ЭНЕРГОСБЕРЕЖЕНИЕ
// ============================================================
void PCFKeypad::_enterPowerSave() {
    if (_powerSaveMode || !_powerSaveEnabled) return;
    _writePCF(_idleMask);
    _powerSaveMode = true;
    _lastIdleTime = millis();
}

void PCFKeypad::_exitPowerSave() {
    if (!_powerSaveMode) return;
    _writePCF(_idleMask);
    _powerSaveMode = false;
    _lastActivityTime = millis();
}

void PCFKeypad::setPowerSave(bool enable) {
    _powerSaveEnabled = enable;
    if (!enable && _powerSaveMode) {
        _exitPowerSave();
    }
}

void PCFKeypad::setPowerSaveTimeout(uint32_t timeout) {
    _powerSaveTimeout = timeout;
}

bool PCFKeypad::isPowerSaveMode() {
    return _powerSaveMode;
}

bool PCFKeypad::_checkActivity() {
    if (!_powerSaveEnabled) return true;

    if (_powerSaveMode) {
        if (_atomicGetChanged()) {
            _exitPowerSave();
            return true;
        }
        return false;
    }

    uint32_t now = millis();
    if (_keyPressed) {
        _lastActivityTime = now;
        return true;
    }

    if (now - _lastActivityTime > _powerSaveTimeout) {
        _enterPowerSave();
        return false;
    }

    return true;
}

// ============================================================
// ЗАЩИТА ОТ GHOSTING
// ============================================================
bool PCFKeypad::_checkGhosting(uint8_t row, uint8_t col) {
    for (uint8_t r = 0; r < 4; r++) {
        if (r == row) continue;

        _writePCF(_rowMasks[r]);
        delayMicroseconds(200);
        byte state = _readPCF();
        _writePCF(_idleMask);   // сброс СРАЗУ после чтения

        if (_lastError != I2C_OK) {
            return true;        // при ошибке считаем ghosting
        }

        bool pressed = _activeLow ?
            ((state & _colMasks[col]) == 0) :
            ((state & _colMasks[col]) != 0);

        if (pressed) {
            return true;
        }
    }
    return false;
}

// ============================================================
// ПРОВЕРКА НАЖАТИЯ ЛЮБОЙ КЛАВИШИ
// ============================================================
bool PCFKeypad::_isAnyKeyPressed() {
    if (!_checkActivity()) {
        return false;
    }

    uint32_t now = millis();
    uint32_t interval = _powerSaveMode ? _scanIntervalIdle : _scanInterval;

    // Своя метка времени — не конфликтует с _scanKey()
    if (_lastAnyKeyScanTime != 0 && (now - _lastAnyKeyScanTime) < interval) {
        return _keyPressed;
    }
    _lastAnyKeyScanTime = now;

    for (uint8_t r = 0; r < 4; r++) {
        _writePCF(_rowMasks[r]);
        delayMicroseconds(200);
        byte state = _readPCF();
        _writePCF(_idleMask);

        if (_lastError != I2C_OK) {
            return _keyPressed;
        }

        for (uint8_t c = 0; c < 4; c++) {
            bool pressed = _activeLow ?
                ((state & _colMasks[c]) == 0) :
                ((state & _colMasks[c]) != 0);

            if (pressed) {
                if (!_powerSaveMode) {
                    _lastActivityTime = millis();
                }
                return true;
            }
        }
    }
    return false;
}

// ============================================================
// СКАНИРОВАНИЕ (С ЗАЩИТОЙ ОТ МНОЖЕСТВЕННОГО НАЖАТИЯ)
// ============================================================
char PCFKeypad::_scanKey() {
    if (_powerSaveMode) {
        return 0;
    }

    uint32_t now = millis();
    if (_lastScanTime != 0 && (now - _lastScanTime) < _scanInterval) {
        return _currentKey;
    }
    _lastScanTime = now;

    char foundKey = 0;
    uint8_t pressedCount = 0;
    uint8_t foundRow = 0;
    uint8_t foundCol = 0;

    for (uint8_t r = 0; r < 4; r++) {
        _writePCF(_rowMasks[r]);
        delayMicroseconds(200);
        byte state = _readPCF();
        _writePCF(_idleMask);

        if (_lastError != I2C_OK) {
            return KEY_ERROR;
        }

        for (uint8_t c = 0; c < 4; c++) {
            bool pressed = _activeLow ?
                ((state & _colMasks[c]) == 0) :
                ((state & _colMasks[c]) != 0);

            if (pressed) {
                foundKey = _keys[r][c];
                foundRow = r;
                foundCol = c;
                pressedCount++;
            }
        }
    }

    // Игнорируем множественное нажатие
    if (pressedCount > 1) {
        return 0;
    }

    // Проверка ghosting для одиночного нажатия
    if (pressedCount == 1) {
        if (_checkGhosting(foundRow, foundCol)) {
            return 0;
        }
        _lastActivityTime = millis();
        return foundKey;
    }

    return 0;
}

// ============================================================
// ОСНОВНОЙ МЕТОД ПОЛУЧЕНИЯ КЛАВИШИ
// ============================================================
char PCFKeypad::getKey() {
    if (!isConnected()) {
        _lastError = I2C_NO_DEVICE;
        return KEY_ERROR;
    }

    if (_powerSaveMode && _atomicGetChanged()) {
        _exitPowerSave();
    }

    uint32_t currentTime = millis();

    if (_useInterrupt) {
        if (!_atomicGetChanged() && !_keyPressed && !_powerSaveMode) {
            return KEY_RELEASED;
        }
    }

    if (!_isAnyKeyPressed()) {
        if (_keyPressed) {
            noInterrupts();
            _keyPressed = false;
            _currentKey = 0;
            interrupts();

            _holdStartTime = 0;
            _lastRepeatTime = 0;
            _atomicSetChanged(false);
            _lastActivityTime = currentTime;
            return KEY_RELEASED;
        }
        _atomicSetChanged(false);
        return KEY_RELEASED;
    }

    char scannedKey = _scanKey();

    if (scannedKey == KEY_ERROR) {
        return KEY_ERROR;
    }

    if (scannedKey != 0) {
        if (!_keyPressed) {
            if (currentTime - _lastPressTime > _debounceDelay) {
                noInterrupts();
                _keyPressed = true;
                _currentKey = scannedKey;
                interrupts();

                _holdStartTime = currentTime;
                _lastRepeatTime = currentTime;
                _lastActivityTime = currentTime;
                _atomicSetChanged(false);
                return KEY_PRESSED;
            }
        } else if (scannedKey == _currentKey) {
            if (currentTime - _holdStartTime > _holdDelay) {
                if (currentTime - _lastRepeatTime > _repeatDelay) {
                    _lastRepeatTime = currentTime;
                    _lastActivityTime = currentTime;
                    _atomicSetChanged(false);
                    return KEY_HOLD;
                }
            }
        } else {
            noInterrupts();
            _keyPressed = false;
            _currentKey = scannedKey;
            interrupts();

            _holdStartTime = currentTime;
            _lastRepeatTime = currentTime;
            _lastPressTime = currentTime;
            _lastActivityTime = currentTime;
            _atomicSetChanged(false);
            return KEY_PRESSED;
        }
    } else {
        if (_keyPressed && _lastError == I2C_OK) {
            noInterrupts();
            _keyPressed = false;
            _currentKey = 0;
            interrupts();

            _holdStartTime = 0;
            _lastRepeatTime = 0;
            _atomicSetChanged(false);
            return KEY_RELEASED;
        }
    }

    return KEY_RELEASED;
}

// ============================================================
// ДОПОЛНИТЕЛЬНЫЕ МЕТОДЫ
// ============================================================
bool PCFKeypad::isKeyPressed() {
    return _isAnyKeyPressed();
}

char PCFKeypad::getCurrentKey() {
    noInterrupts();
    char k = _currentKey;
    interrupts();
    return k;
}

uint8_t PCFKeypad::getLastError() {
    return _lastError;
}

// ============================================================
// НАСТРОЙКА
// ============================================================
void PCFKeypad::setDebounceTime(uint16_t ms) {
    _debounceDelay = ms;
}

void PCFKeypad::setHoldDelay(uint16_t ms) {
    _holdDelay = ms;
}

void PCFKeypad::setRepeatDelay(uint16_t ms) {
    _repeatDelay = ms;
}

void PCFKeypad::setScanInterval(uint16_t ms) {
    if (ms < 1) ms = 1;
    _scanInterval = ms;
    _scanIntervalIdle = ms * 5;
}

void PCFKeypad::setActiveLow(bool activeLow) {
    _activeLow = activeLow;
    _calculateMasks();
}