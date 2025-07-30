<<<<<<< Updated upstream
#include <Arduino.h>
#include <DHTesp.h>

// BAUDRATE
#define baudrate 115200

// Defina os pinos de LED e LDR e DHT
#define DHT_PIN 12
#define LED_PIN 22
#define LDR_PIN 36

// Defina uma variável com valor máximo do LDR (4000)
int ldrMax = 4095;

// Defina uma variável para guardar o valor atual do LED (10)
int ledValue = 10;

//Configurando PWM
=======
int ledPin = 22;
int ledValue = 10;

int ldrPin = 34;
int ldrMax = 4095;

>>>>>>> Stashed changes
const int pwmFreq = 5000;
const int pwmResolution = 8;

// Objeto para manipular sensor DHT
DHTesp dht;

void setup() {
<<<<<<< Updated upstream
    Serial.begin(baudrate);
    pinMode(LED_PIN, OUTPUT);
    pinMode(LDR_PIN, INPUT);
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(LED_PIN, pwmChannel);
    ledcWrite(pwmChannel, 0);
    analogReadResolution(12);
    dht.setup(DHT_PIN, DHTesp::DHT22);
    Serial.printf("SmartLamp Initialized.\n");
=======
    Serial.begin(9600);
    
    //ledcAttach(ledPin, pwmFreq, pwmResolution); nós achamos precisava
    
    pinMode(ldrPin, INPUT);
    pinMode(ledPin, OUTPUT);
    
    ledUpdate();
>>>>>>> Stashed changes
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            processCommand(command);
        }
    }

    processCommand("GET_LDR");
}

void processCommand(String command) {
<<<<<<< Updated upstream
    // compare o comando com os comandos possíveis e execute a ação correspondente
    if (command.startsWith("SET_LED")) {
      String valueStr = command.substring(command.indexOf(' ') + 1);
      if(valueStr.toInt() >= 0 && valueStr.toInt() <= 100) {
        brightness = valueStr.toInt();
        ledUpdate();
        Serial.println("RES SET_LED 1");
      } else {
        Serial.println("RES SET_LED -1");
      }
    } else if(command.startsWith("GET_LED")){
      Serial.print("RES GET_LED ");
      Serial.println(brightness);
    } else if(command.startsWith("GET_LDR")){
      int ldrValue = ldrGetValue();
      Serial.print("RES GET_LDR ");
      Serial.println(ldrValue);
    } else if(command.startsWith("GET_TEMP")) {
      float temp = dht.getTemperature();
      Serial.print("RES GET_TEMP ");
      Serial.println(temp);
    } else if(command.startsWith("GET_HUM")) { 
      float hum = dht.getHumidity();
      Serial.print("RES GET_HUM ");
      Serial.println(hum);
=======
    int spaceIndex = command.indexOf(' ');
    String cmd, valueStr;
    int value = 0;

    if (spaceIndex != -1) {
        cmd = command.substring(0, spaceIndex);
        valueStr = command.substring(spaceIndex + 1);
        value = valueStr.toInt();
>>>>>>> Stashed changes
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
    } else if (cmd == "GET_LDR") {
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
<<<<<<< Updated upstream
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
  int raw = analogRead(LDR_PIN);
  int normalized = map(raw, 0, ldrMax, 0, 100);
  return constrain(normalized, 0, 100);
}
=======
    int rawValue = analogRead(ldrPin);
    int normalizedValue = map(rawValue, 0, ldrMax, 0, 100);
    if (normalizedValue > 100) normalizedValue = 100;
    if (normalizedValue < 0) normalizedValue = 0;
    return normalizedValue;
}
>>>>>>> Stashed changes
