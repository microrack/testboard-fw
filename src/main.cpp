#include <Arduino.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_MCP23X17.h>

#include <DAC8552.h>

const int SCL_PIN = 22;
const int SDA_PIN = 21;

const int IO0_RST = 23;
const int IO1_RST = 19;

const int PIN_MOSI = 18;
const int PIN_SCK  = 5;
const int PIN_CS1  = 17;  // TX2 → DAC8552 #1
const int PIN_CS2  = 16;  // RX2 → DAC8552 #2


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

SPIClass SPI_DAC(HSPI);
DAC8552 dac1(PIN_CS1, &SPI_DAC);
DAC8552 dac2(PIN_CS2, &SPI_DAC);

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

    // Инициализация пинов CS
    pinMode(PIN_CS1, OUTPUT);
    pinMode(PIN_CS2, OUTPUT);
    digitalWrite(PIN_CS1, HIGH);
    digitalWrite(PIN_CS2, HIGH);

    // Инициализация SPI
    SPI_DAC.begin(PIN_SCK, -1, PIN_MOSI, -1); // SCK, MISO (нет), MOSI, SS (не используется)
    dac1.begin();
    dac2.begin();

    // Вывод в консоль
    Serial.println("Hello, Microrack!");
}

void writeDAC(int cs_pin, uint8_t channel, uint16_t value) {
    uint8_t command = 0x00;
    if (channel == 0) command = 0x00;         // DAC A
    else if (channel == 1) command = 0x10;    // DAC B
    else if (channel == 2) command = 0x20;    // both (A+B)

    uint8_t highByte = (value >> 8) & 0xFF;
    uint8_t lowByte  = value & 0xFF;

    digitalWrite(cs_pin, LOW);
    SPI_DAC.transfer(command);
    SPI_DAC.transfer(highByte);
    SPI_DAC.transfer(lowByte);
    digitalWrite(cs_pin, HIGH);
    delayMicroseconds(100);
}

void loop() {
    static bool state = false;

    mcp1.digitalWrite(LED1_PIN, state);
    mcp1.digitalWrite(LED2_PIN, !state);

    // Serial.println(state ? "LED1 ON, LED2 OFF" : "LED1 OFF, LED2 ON");

    Serial.printf("+12: %d +5: %d -12: %d\n",
        mcp1.digitalRead(P12V_PASS),
        mcp1.digitalRead(P5V_PASS),
        !mcp1.digitalRead(M12V_PASS)
    );

    static uint16_t value = 0;
    dac1.setValue(0, value);
    dac1.setValue(1, value);
    dac2.setValue(0, value);
    dac2.setValue(1, value);
    value += 5000;
    if(value > 65536 - 5000) {
        value = 0;
    }

    state = !state;
    delay(500);
}
