

#ifndef display_H
#define display_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <telemetry_manager.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define NUM_GRAINS 150    // Количество песчинок (не используется, но оставлено)
#define GRAIN_SIZE 4      // Размер элемента (не используется)
#define I2C_SPEED 1000000 // Увеличиваем частоту I2C до 1 МГц
#define SAND_TOP_LIMIT 20 // Верхний предел, где не должно быть круга

extern Adafruit_SSD1306 display;
// Функция для примера
void updateDisplay();
void addTopBar();
void telemetryDisplay();
void printWithSpace(const String &input, int expectedLength);
void showMessage(const String &message);
void setBrightness(uint8_t brightness);

extern int8_t display_mode;
#endif