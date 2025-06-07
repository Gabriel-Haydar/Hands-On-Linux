// Defina os pinos de LED e LDR
// Defina uma variável com valor máximo do LDR (4000)
// Defina uma variável para guardar o valor atual do LED (10)
const int ledPin = 22;
int ledValue = 10;

const int ldrPin = 36;
// Faça testes no sensor ldr para encontrar o valor maximo e atribua a variável ldrMax
int ldrMax = 4000;

//Configurando PWM
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;
int brightness = 0;

void setup() {
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    pinMode(ldrPin, INPUT);
    ledcSetup(pwmChannel, pwmFreq, pwmResolution);
    ledcAttachPin(ledPin, pwmChannel);
    ledcWrite(pwmChannel, 0);
    analogReadResolution(12);
    Serial.printf("SmartLamp Initialized.\n");
}

// Função loop será executada infinitamente pelo ESP32
void loop() {
    //Obtenha os comandos enviados pela serial 
    //e processe-os com a função processCommand
    if(Serial.available() > 0) {
      String command = Serial.readStringUntil('\n');
      processCommand(command);
    }
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
    } else {
      Serial.println("ERR Unknown command");
    }
}       

// Função para atualizar o valor do LED
void ledUpdate() {
  // Valor deve convertar o valor recebido pelo comando SET_LED para 0 e 255
  // Normalize o valor do LED antes de enviar para a porta correspondente
  int dutCycle = map(brightness, 0, 100, 0, 255);
  ledcWrite(pwmChannel, dutCycle);
}

// Função para ler o valor do LDR
int ldrGetValue() {
    // Leia o sensor LDR e retorne o valor normalizado entre 0 e 100
    // faça testes para encontrar o valor maximo do ldr (exemplo: aponte a lanterna do celular para o sensor)       
    // Atribua o valor para a variável ldrMax e utilize esse valor para a normalização
    return analogRead(ldrPin);
}