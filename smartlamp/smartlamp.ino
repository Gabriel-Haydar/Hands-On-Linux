#include <DHT.h>

int ledPin = 22;
int ledValue = 10;

#define DHTPIN 4
int ldrPin = 34;
int ldrMax = 4095;

const int pwmFreq = 5000;
const int pwmResolution = 8;

#define DHTTYPE DHT11 


DHT dht(DHTPIN, DHTTYPE);
void setup() {
    Serial.begin(9600);
    
    //ledcAttach(ledPin, pwmFreq, pwmResolution); nós achamos precisava
    
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);
    
    ledUpdate();
    dht.begin();
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            processCommand(command);
        }
    }

    //processCommand("GET_LDR");
}

void processCommand(String command) {
    int spaceIndex = command.indexOf(' ');
    String cmd, valueStr;
    int value = 0;

    if (spaceIndex != -1) {
        cmd = command.substring(0, spaceIndex);
        valueStr = command.substring(spaceIndex + 1);
        value = valueStr.toInt();
    } else {
        cmd = command;
    }

    if (cmd == "SET_LED") {
        if (value >= 0 && value <= 100) {
            ledValue = value;
            ledUpdate();
            Serial.println("RES SET_LED 1");
        } else {
            Serial.println("RES SET_LED -1");
        }
    } else if (cmd == "GET_LED") {
        Serial.print("RES GET_LED ");
        Serial.println(ledValue);
    }
     else if (cmd == "GET_TEMP") {
        Serial.print("RES GET_TEMP ");
        float tempValue = tempGetValue();
        Serial.println(tempValue);
    }  else if (cmd == "GET_HUM") {
        Serial.print("RES GET_HUM ");
        float humValue = umidGetValue();
        Serial.println(humValue);
    } 
     else if (cmd == "GET_LDR") {
        int ldrValue = ldrGetValue();
        Serial.println("RES GET_LDR " + String(ldrValue));
    } else {
        Serial.println("ERR Unknown command.");
    }
}

void ledUpdate() {
    int pwmValue = map(ledValue, 0, 100, 0, 255);
    analogWrite(ledPin, pwmValue);
}

int ldrGetValue() {
    int rawValue = analogRead(ldrPin);
    int normalizedValue = map(rawValue, 0, ldrMax, 0, 100);
    if (normalizedValue > 100) normalizedValue = 100;
    if (normalizedValue < 0) normalizedValue = 0;
    return normalizedValue;
}

float tempGetValue(){
  float t = dht.readTemperature();

  return t;
}

float umidGetValue(){
  float h = dht.readHumidity();

  return h;
}
