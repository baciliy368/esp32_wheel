/*
 * This example turns the ESP32 into a Bluetooth LE gamepad that presses buttons and moves axis
 *
 * At the moment we are using the default settings, but they can be canged using a BleGamepadConfig instance as parameter for the begin function.
 *
 * Possible buttons are:
 * BUTTON_1 through to BUTTON_16
 * (16 buttons by default. Library can be configured to use up to 128)
 *
 * Possible DPAD/HAT switch position values are:
 * DPAD_CENTERED, DPAD_UP, DPAD_UP_RIGHT, DPAD_RIGHT, DPAD_DOWN_RIGHT, DPAD_DOWN, DPAD_DOWN_LEFT, DPAD_LEFT, DPAD_UP_LEFT
 * (or HAT_CENTERED, HAT_UP etc)
 *
 * bleGamepad.setAxes sets all axes at once. There are a few:
 * (x axis, y axis, z axis, rx axis, ry axis, rz axis, slider 1, slider 2)
 *
 * Library can also be configured to support up to 5 simulation controls
 * (rudder, throttle, accelerator, brake, steering), but they are not enabled by default.
 *
 * Library can also be configured to support different function buttons
 * (start, select, menu, home, back, volume increase, volume decrease, volume mute)
 * start and select are enabled by default
 */

#include <Arduino.h>
#include <BleGamepad.h>
#include <EncButton.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <display.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <telemetry_manager.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

const char *ssid = "dog_room";
const char *password = "NGHaUH8H88";
BleGamepad bleGamepad;
const int buttonPins[2] = {4, 14}; // Замените на свои пины
// const int buttonMappings[7] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4, BUTTON_5, BUTTON_6, BUTTON_7};
const int buttonMappings[2] = {BUTTON_1, BUTTON_2};

#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels

EncButton eb(12, 13, 27, INPUT_PULLUP);

const char *ws_server = "192.168.0.102"; // IP или домен WebSocket-сервера
const int ws_port = 8888;                // Порт WebSocket-сервера
const char *ws_path = "/ws";
WebSocketsClient webSocket;

// OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool buttonStates[2] = {false, false}; // Состояния кнопок

void callback()
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("callback: ");

  switch (eb.action())
  {
  case EB_PRESS:
  case EB_CLICK:
    display.println("press");
    break;
  case EB_HOLD:
    display.println("hold");
    break;
  case EB_TURN:
    display.print("turn ");
    display.print(eb.dir());
    display.print(" ");
    display.print(eb.fast());
    display.print(" ");
    display.println(eb.pressing());
    break;
  }
  display.display();
}

// Функция, вызываемая при подключении WebSocket-клиента
void onWebSocketConnected()
{
  showMessage("Клиент подключился к WebSocket");
  webSocket.sendTXT("echo|");
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_CONNECTED)
  {
    onWebSocketConnected();
  }
  else if (type == WStype_TEXT)
  {

    String m = String((char *)payload);
    /* The `showMessage` function is a custom function used in the code to display messages on an OLED
    display. It takes a string message as input and displays it on the OLED screen. The function is
    used to provide status updates or information during the execution of the program. */

    if (m.indexOf("-1|echo") != -1)
    {
      webSocket.sendTXT("pid|-1");
      webSocket.sendTXT("mainTemplateLoading|");
      webSocket.sendTXT("registerComponents|/Dashtemplates/123/123.djson");
      webSocket.sendTXT("mainTemplateLoaded|");
    }

    else
    {
      String payload_str = String((char *)payload);
      StaticJsonDocument<300> doc;

      // Извлекаем JSON-часть сообщения
      String jsonData = payload_str.substring(payload_str.indexOf('|') + 1);

      // Десериализация JSON
      DeserializationError error = deserializeJson(doc, jsonData);
      showMessage(payload_str);

      if (!error)
      {
        updateTelemetry(doc); // Обновляем телеметрию

        int number = payload_str.substring(0, payload_str.indexOf('|')).toInt();

        // Отправляем обратно увеличенное число
        String response = "pid|" + String(number+1);
        webSocket.sendTXT(response);
      }
    }
  }
}

void otaLogic()
{
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
        type = "sketch";
      } else {  // U_SPIFFS
        type = "filesystem";
      }

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
        Serial.println("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
        Serial.println("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
        Serial.println("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
        Serial.println("Receive Failed");
      } else if (error == OTA_END_ERROR) {
        Serial.println("End Failed");
      } });

  ArduinoOTA.begin();
}

void bleTask(void *pvParameters)
{
  while (true)
  {
    if (bleGamepad.isConnected())
    {
      eb.tick();
      for (int i = 0; i < 2; i++)
      {
        if (digitalRead(buttonPins[i]) == LOW && !buttonStates[i])
        {
          bleGamepad.press(buttonMappings[i]); // Нажатие кнопки
          buttonStates[i] = true;              // Запоминаем, что кнопка нажата
        }
        else if (digitalRead(buttonPins[i]) == HIGH && buttonStates[i])
        {
          bleGamepad.release(buttonMappings[i]); // Отпускание кнопки
          buttonStates[i] = false;               // Запоминаем, что кнопка отпущена
        }
      }
    }
    else
    {
      showMessage("READY FOR CONNECTION");
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Проверка каждые 5 секунд
  }
}

void otherTask(void *pvParameters)
{
  while (true)
  {
    ArduinoOTA.handle();
    webSocket.loop();
    updateDisplay();
    vTaskDelay(pdMS_TO_TICKS(1)); // Проверка каждые 5 секунд
  }
}
void setup()
{
  Serial.begin(9600);
  WiFi.begin(ssid, password);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(WHITE);
  display.clearDisplay();
  otaLogic();
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    showMessage("WIFI Connection");
  }
  Serial.println(WiFi.localIP());

  webSocket.begin(ws_server, ws_port, ws_path);
  webSocket.onEvent([](WStype_t type, uint8_t *payload, size_t length)
                    { webSocketEvent(type, payload, length); });
  webSocket.setReconnectInterval(5000); // Автоматическое переподключение каждые 5 секунд

  // Настройка пинов кнопок
  for (int i = 0; i < 2; i++)
  {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  eb.attach(callback);
  eb.setEncType(EB_STEP2);
  showMessage("Starting BLE work!");
  bleGamepad.begin();

  showMessage("READY FOR CONNECTION");
  setBrightness(170); // Максимальная яркость

  xTaskCreatePinnedToCore(otherTask, "otherTask", 4096, NULL, 1, NULL, 1); // OTA на ядре 0
  xTaskCreatePinnedToCore(bleTask, "bleTask", 4096, NULL, 2, NULL, 0);
}

void loop()
{
}
