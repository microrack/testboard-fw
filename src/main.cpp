#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>

const int SCL_PIN = 22;
const int SDA_PIN = 21;

const int IO0_RST = 23;
const int IO1_RST = 19;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

Adafruit_MCP23X17 mcp0;  // addr 0x20
Adafruit_MCP23X17 mcp1;  // addr 0x21

const int GPB = 8;
const int LED1_PIN = GPB + 3;
const int LED2_PIN = GPB + 4;

const int P12V_PASS = GPB + 0;
const int P5V_PASS = GPB + 1;
const int M12V_PASS = GPB + 2;

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

  pinMode(IO0_RST, OUTPUT);
  digitalWrite(IO0_RST, LOW);   // удерживаем в сбросе
  delay(10);
  digitalWrite(IO0_RST, HIGH);  // выходим из сброса

  pinMode(IO1_RST, OUTPUT);
  digitalWrite(IO1_RST, LOW);   // удерживаем в сбросе
  delay(10);
  digitalWrite(IO1_RST, HIGH);  // выходим из сброса

  mcp1.begin_I2C(0x21);
  mcp1.pinMode(LED1_PIN, OUTPUT);
  mcp1.pinMode(LED2_PIN, OUTPUT);

  mcp1.pinMode(P12V_PASS, INPUT);
  mcp1.pinMode(P5V_PASS, INPUT);
  mcp1.pinMode(M12V_PASS, INPUT);

  // Вывод в консоль
  Serial.println("Hello, Microrack!");
}

void loop() {
  static bool state = false;

  mcp1.digitalWrite(LED1_PIN, state);
  mcp1.digitalWrite(LED2_PIN, !state);

  // Serial.println(state ? "LED1 ON, LED2 OFF" : "LED1 OFF, LED2 ON");

  Serial.printf("+12: %d +5: %d -12: %d\n",
    mcp1.digitalRead(P12V_PASS),
    mcp1.digitalRead(P5V_PASS),
    mcp1.digitalRead(M12V_PASS)
  );

  state = !state;
  delay(500);
}
