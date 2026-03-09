#define DO_PIN 18

void setup() {
    Serial.begin(115200);
    pinMode(DO_PIN, INPUT);
}

void loop() {
    int lightVal = digitalRead(DO_PIN);

    if (lightVal == LOW) {
        Serial.println("Light detected!");
    } else {
        Serial.println("Dark!");
    }

    delay(500);
}