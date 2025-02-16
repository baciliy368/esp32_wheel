#include "display.h"
#include <Wire.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define I2C_SPEED 1000000    // Увеличиваем частоту I2C до 1 МГц
#define UPDATE_INTERVAL 400  // Обновление каждые 400 мс
#define ARROW_LENGTH 20      // Увеличенная длина стрелки
#define ARROW_HEAD_SIZE 5    // Увеличенный размер наконечника стрелки
#define ARROW_THICKNESS 2    // Толщина стрелки
#define ANGLE_TOLERANCE 0.05 // Игнорировать изменения < 5%

// **Координаты колес в нормализованном виде**
const float wheelX[] = {-1, 1, -1, 1}; // FL, FR, RL, RR
const float wheelY[] = {1, 1, -1, -1}; // FL, FR, RL, RR

// **Координаты стрелки (зона передачи)**
#define GEAR_POS_X 49
#define GEAR_POS_Y 20

int8_t display_mode = 0;
unsigned long lastUpdateTime = 0;
float currentAngle = 0; // Текущий угол стрелки (градусы)

void showMessage(const String &message)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(message);
    display.display();
}

void setBrightness(uint8_t brightness)
{
    if (brightness > 255)
        brightness = 255; // Ограничиваем диапазон
    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(brightness);
}

void addTyre()
{
    display.setTextSize(2, 2);
    display.setCursor(0, 20);
    printWithSpace(telemetry.tyre_f_l, 3);
    display.setCursor(92, 20);
    display.print(telemetry.tyre_f_r);
    display.setCursor(0, 48);
    printWithSpace(telemetry.tyre_r_l, 3);
    display.setCursor(92, 48);
    display.print(telemetry.tyre_r_r);
}

// **Определение направления стрелки**
float calculateAngle(TelemetryData telemetry)
{
    // Массив прироста массы для всех колес
    float loadIncrease[4] = {
        telemetry.tyre_f_l_m, // FL
        telemetry.tyre_f_r_m, // FR
        telemetry.tyre_r_l_m, // RL
        telemetry.tyre_r_r_m  // RR
    };

    // Определяем колесо с максимальным приростом массы
    int maxWheel = 0;
    float maxLoad = loadIncrease[0];

    for (int i = 1; i < 4; i++)
    {
        if (loadIncrease[i] > maxLoad)
        {
            maxLoad = loadIncrease[i];
            maxWheel = i;
        }
    }

    // Определяем второе колесо, если прирост ≥ 70% от первого
    int secondWheel = -1;
    float secondLoad = 0;

    for (int i = 0; i < 4; i++)
    {
        if (i != maxWheel && loadIncrease[i] >= 0.7 * maxLoad)
        {
            secondWheel = i;
            secondLoad = loadIncrease[i];
            break;
        }
    }

    // Определяем координаты направления
    float cx, cy;
    if (secondWheel == -1)
    {
        // Если только одно колесо с приростом
        cx = wheelX[maxWheel];
        cy = wheelY[maxWheel];
    }
    else
    {
        // Если есть второе колесо, берем среднюю точку между ними
        cx = (wheelX[maxWheel] + wheelX[secondWheel]) / 2;
        cy = (wheelY[maxWheel] + wheelY[secondWheel]) / 2;
    }

    // Рассчитываем угол (atan2 возвращает радианы, переводим в градусы)
    float angle = atan2(-cy, cx) * 180.0 / M_PI;
    return angle;
}

// **Рисование толстой стрелки с наконечником**
void drawArrow(float angle)
{
    // Центр стрелки совпадает с положением передачи
    int x0 = GEAR_POS_X + 15; // Центрируем стрелку в символе
    int y0 = GEAR_POS_Y + 18; // Смещаем немного вниз по центру цифры

    // Вычисляем конечную точку стрелки
    int x1 = x0 + ARROW_LENGTH * cos(angle * M_PI / 180.0);
    int y1 = y0 + ARROW_LENGTH * sin(angle * M_PI / 180.0);

    // Добавляем толщину стрелке (дублируем линии по бокам)
    for (int i = -ARROW_THICKNESS / 2; i <= ARROW_THICKNESS / 2; i++)
    {
        display.drawLine(x0 + i, y0, x1 + i, y1, SSD1306_WHITE);
        display.drawLine(x0, y0 + i, x1, y1 + i, SSD1306_WHITE);
    }

    // Вычисляем точки для наконечника
    float leftAngle = angle + 135;  // Левый угол наконечника
    float rightAngle = angle - 135; // Правый угол наконечника

    int xLeft = x1 + ARROW_HEAD_SIZE * cos(leftAngle * M_PI / 180.0);
    int yLeft = y1 + ARROW_HEAD_SIZE * sin(leftAngle * M_PI / 180.0);

    int xRight = x1 + ARROW_HEAD_SIZE * cos(rightAngle * M_PI / 180.0);
    int yRight = y1 + ARROW_HEAD_SIZE * sin(rightAngle * M_PI / 180.0);

    // Рисуем наконечник
    display.drawLine(x1, y1, xLeft, yLeft, SSD1306_WHITE);
    display.drawLine(x1, y1, xRight, yRight, SSD1306_WHITE);
}

void massDisplay()
{
    display.clearDisplay();
    addTopBar();
    addTyre();

    // Получаем новый угол
    float targetAngle = calculateAngle(telemetry);

    // Проверяем, изменился ли угол более чем на 5%
    if (abs(targetAngle - currentAngle) / abs(currentAngle + 0.01) > ANGLE_TOLERANCE) // +0.01 защита от деления на 0
    {
        currentAngle = targetAngle;
    }

    drawArrow(currentAngle);
    display.display();
}

// **Методы, не относящиеся к задаче (сохранены)**

void printWithSpace(const String &input, int expectedLength)
{
    if (input.length() < expectedLength)
    {
        for (size_t i = input.length(); i < expectedLength; i++)
        {
            display.print(' ');
        }
    }
    display.print(input);
}

void addTopBar()
{
    display.setTextSize(2, 2);
    display.setCursor(0, 0);
    printWithSpace(telemetry.tc, 2);
    display.setCursor(40, 0);
    display.print(telemetry.bias);
    display.setCursor(104, 0);
    display.print(telemetry.abs);
}

void addGear()
{
    display.setTextSize(6);
    display.setCursor(GEAR_POS_X, GEAR_POS_Y);
    display.print(telemetry.gear);
}

void telemetryDisplay()
{
    display.clearDisplay();
    addTopBar();
    addTyre();
    addGear();
    display.display();
}

void updateDisplay()
{
    if (telemetry.isValid())
    {
        switch (display_mode)
        {
        case 0:
            telemetryDisplay();
            break;
        case 1:
            massDisplay();
            break;
        default:
            break;
        }
    }
}
