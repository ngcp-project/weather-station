#include <DHT.h>

#define DHTPIN 21
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup() {
    Serial.begin(115200);
    dht.begin();
    Serial.println("DHT22 Sensor Ready!");
}

void loop() {
    delay(2000);  // DHT22 needs at least 2 seconds between readings

    float humidity    = dht.readHumidity();
    float tempC       = dht.readTemperature();
    float tempF       = dht.readTemperature(true);  // true = Fahrenheit
    float heatIndex   = dht.computeHeatIndex(tempF, humidity);

    // Check if readings are valid
    if (isnan(humidity) || isnan(tempC)) {
        Serial.println("Failed to read from DHT22!");
        return;
    }

    Serial.println("-----------------------------");
    Serial.print("Humidity:    ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Temp (C):    ");
    Serial.print(tempC);
    Serial.println(" °C");

    Serial.print("Temp (F):    ");
    Serial.print(tempF);
    Serial.println(" °F");

    Serial.print("Heat Index:  ");
    Serial.print(heatIndex);
    Serial.println(" °F");
}