/**
 * Weather Station — ESP32
 *
 * Sensors:
 *  - BH1750   (I2C)    Light intensity        SDA=22, SCL=23
 *  - DHT22    (GPIO21)  Temperature & Humidity
 *  - LM393    (GPIO18)  Light trigger
 *  - SEN0482  (RS485)   Wind Direction
 */

#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>

// ── Existing sensor pins ──────────────────────────────────────
#define DHT_PIN       21
#define LM393_PIN     18
#define DHT_TYPE      DHT22

// ── SEN0482 RS485 pins ────────────────────────────────────────
#define RS485_RX_PIN  16    // RX2
#define RS485_TX_PIN  17    // TX2
#define RS485_DE_PIN   5    // RE+DE direction control

// ── RS485 config ──────────────────────────────────────────────
#define RS485_SERIAL  Serial2
#define RS485_BAUD    9600

// Modbus RTU request for SEN0482
const uint8_t WIND_DIR_REQUEST[] = { 0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x39 };

const char* COMPASS_16[16] = {
  "N", "NNE", "NE", "ENE",
  "E", "ESE", "SE", "SSE",
  "S", "SSW", "SW", "WSW",
  "W", "WNW", "NW", "NNW"
};

BH1750 lightMeter;
DHT dht(DHT_PIN, DHT_TYPE);

// ── CRC-16 Modbus ─────────────────────────────────────────────
uint16_t crc16Modbus(const uint8_t* data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i];
    for (uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x0001) ? (crc >> 1) ^ 0xA001 : crc >> 1;
    }
  }
  return crc;
}

// ── Degrees to compass label ──────────────────────────────────
String degreesToCompass(float deg) {
  deg = fmod(deg, 360.0f);
  if (deg < 0) deg += 360.0f;
  return String(COMPASS_16[(int)((deg + 11.25f) / 22.5f) % 16]);
}

// ── Read wind direction from SEN0482 ──────────────────────────
bool readWindDirection(float& outDegrees) {
  while (RS485_SERIAL.available()) RS485_SERIAL.read();

  digitalWrite(RS485_DE_PIN, HIGH);
  delayMicroseconds(100);
  RS485_SERIAL.write(WIND_DIR_REQUEST, sizeof(WIND_DIR_REQUEST));
  RS485_SERIAL.flush();
  delayMicroseconds(100);
  digitalWrite(RS485_DE_PIN, LOW);

  uint8_t response[7];
  uint8_t bytesRead = 0;
  uint32_t start = millis();
  while (bytesRead < 7) {
    if (millis() - start > 500) return false;
    if (RS485_SERIAL.available()) response[bytesRead++] = RS485_SERIAL.read();
  }

  uint16_t calcCRC = crc16Modbus(response, 5);
  uint16_t recvCRC = (uint16_t)response[6] << 8 | response[5];
  if (calcCRC != recvCRC) return false;

  uint16_t raw = (uint16_t)response[3] << 8 | response[4];
  outDegrees   = raw / 10.0f;
  return true;
}

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Weather Station ===");

  Wire.begin(22, 23);
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println("[BH1750]  OK");
  } else {
    Serial.println("[BH1750]  FAILED - check wiring");
  }

  dht.begin();
  Serial.println("[DHT22]   OK");

  pinMode(LM393_PIN, INPUT_PULLUP);
  Serial.println("[LM393]   OK");

  pinMode(RS485_DE_PIN, OUTPUT);
  digitalWrite(RS485_DE_PIN, LOW);
  RS485_SERIAL.begin(RS485_BAUD, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  Serial.println("[SEN0482] OK");

  Serial.println("-------------------");
}

void loop() {
  // --- BH1750 ---
  float lux = lightMeter.readLightLevel();
  if (lux >= 0) {
    Serial.print("[BH1750]  Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  } else {
    Serial.println("[BH1750]  Read error");
  }

  // --- DHT22 ---
  float humidity = dht.readHumidity();
  float tempC    = dht.readTemperature();
  float tempF    = dht.readTemperature(true);

  if (isnan(humidity) || isnan(tempC) || isnan(tempF)) {
    Serial.println("[DHT22]   Read error - check wiring");
  } else {
    Serial.print("[DHT22]   Temp: ");
    Serial.print(tempC);
    Serial.print(" C  |  ");
    Serial.print(tempF);
    Serial.print(" F  |  Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  // --- LM393 ---
  int lm393State = digitalRead(LM393_PIN);
  Serial.print("[LM393]   Signal: ");
  Serial.println(lm393State == LOW ? "LIGHT DETECTED (triggered)" : "NO LIGHT (no trigger)");

  // --- SEN0482 ---
  float windDeg = 0.0f;
  if (readWindDirection(windDeg)) {
    Serial.print("[SEN0482] Wind: ");
    Serial.print(windDeg, 1);
    Serial.print("°  (");
    Serial.print(degreesToCompass(windDeg));
    Serial.println(")");
  } else {
    Serial.println("[SEN0482] Read error - check wiring/power");
  }

  Serial.println("-------------------");
  delay(2000);
}