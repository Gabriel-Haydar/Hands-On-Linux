#include <Arduino.h>
#include <DHTesp.h>
// Display Includes
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
// OLED
#define OLED_WIDTH 128 // OLED display width,  in pixels
#define OLED_HEIGHT 32 // OLED display height, in pixels

// BAUDRATE
#define baudrate 115200

// Defina os pinos de LED e LDR e DHT
#define DHT_PIN 4
#define LED_PIN 32
#define LED_PIN2 33
#define LDR_PIN 36


Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// Defina uma variável com valor máximo do LDR (4000)
int ldrMax = 4095;

// Defina uma variável para guardar o valor atual do LED (10)
int ledValue = 10;

// Variável para guardar o valor atual do LDR
int ldrValue = 0;

// Variável para guardar o valor atual de Temperatura
int tempValue = 0;

// Variável para guardar o valor atual de Temperatura
int humValue = 0;

//Configurando PWM
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;
int brightness = 0;

// Objeto para manipular sensor DHT
DHTesp dht;

void setup() {
  Serial.begin(baudrate);
  pinMode(LED_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(LED_PIN, pwmChannel);
  ledcWrite(pwmChannel, 0);
  analogReadResolution(12);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.printf("SmartLamp Initialized.\n");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao iniciar o display OLED"));
    while (true); // trava o código se falhar
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("SmartLamp OLED OK");
  display.display();
}

// Função loop será executada infinitamente pelo ESP32
void loop() {
  //Obtenha os comandos enviados pela serial 
  //e processe-os com a função processCommand
  if(Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim(); // Remove espaços e quebras de linha
    processCommand(command);
  }
  displaySensorValues();
  delay(100);
}


void processCommand(String command) {
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
  } else {
    Serial.println("ERR Unknown command");
  }
}       

// Função para atualizar o valor do LED
void ledUpdate() {
  // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
  // Normalize o valor do LED antes de enviar para a porta correspondente
  int dutyCycle  = map(brightness, 0, 100, 0, 255);
  ledcWrite(pwmChannel, dutyCycle);
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
  int raw = analogRead(LDR_PIN);
  int normalized = map(raw, 0, ldrMax, 0, 100);
  return constrain(normalized, 0, 100);
}

void displaySensorValues() {
  float temp = dht.getTemperature();
  float hum = dht.getHumidity();
  int ldr = ldrGetValue();

  display.clearDisplay();
  display.setCursor(0, 0);

  display.print("Temp: ");
  display.print(temp, 1);
  display.println(" C");

  display.print("Hum: ");
  display.print(hum, 1);
  display.println(" %");

  display.print("LDR: ");
  display.println(ldr);

  display.display();
}
