#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MCP23X17 mcp0;  // addr 0x20
Adafruit_MCP23X17 mcp1;  // addr 0x21

#define LED1_PIN 11  // GPB3 = pin 11 на MCP23017
#define LED2_PIN 12  // GPB4 = pin 12 на MCP23017

void setup() {
  // Инициализация сериал-порта
  Serial.begin(115200);
  delay(1000);  // Небольшая задержка на запуск

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.setRotation(0);
  display.clearDisplay();
  display.setCursor(0, 0);
  display.printf("Hello, Microrack!\n");
  display.display();

  mcp1.begin_I2C(0x21);
  mcp1.pinMode(LED1_PIN, OUTPUT);
  mcp1.pinMode(LED2_PIN, OUTPUT);

  // Вывод в консоль
  Serial.println("Hello, Microrack!");
}

void loop() {
  static bool state = false;

  mcp1.digitalWrite(LED1_PIN, state);
  mcp1.digitalWrite(LED2_PIN, !state);

  Serial.println(state ? "LED1 ON, LED2 OFF" : "LED1 OFF, LED2 ON");

  state = !state;
  delay(500);
}
