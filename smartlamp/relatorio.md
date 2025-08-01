# SmartLamp Wiring Guide (ESP32)

Este documento descreve como conectar corretamente os componentes utilizados no projeto **SmartLamp** com ESP32, incluindo sensor DHT22, LDR, LEDs e o display OLED I2C.

---

## 📌 Resumo dos Pinos Utilizados

| Componente       | Pino ESP32 | Função                |
|------------------|------------|------------------------|
| **DHT22**        | GPIO 4     | Sensor de temperatura e umidade |
| **LED PWM 1**    | GPIO 32    | Saída PWM (canal 0)    |
| **LED PWM 2**    | GPIO 33    | Reservado (não usado no código atual) |
| **LDR (Fotoresistor)** | GPIO 36 (VP) | Entrada analógica (ADC) |
| **Display OLED (I2C)** | GPIO 21 (SDA), GPIO 22 (SCL) | Comunicação I2C |

---

## 🖧 Display OLED 128x32 (I2C)

### 📍 Conexões:

| OLED Pin | Conecta ao ESP32 |
|----------|------------------|
| GND      | GND              |
| VCC      | 3.3V             |
| SCL      | GPIO 22          |
| SDA      | GPIO 21          |

> Certifique-se de que o endereço I2C do display seja `0x3C`, como configurado no código.

---

## 🌡️ Sensor DHT22 (Temperatura e Umidade)

### 📍 Conexões:

| DHT22 Pin | Conecta ao ESP32 |
|-----------|------------------|
| VCC       | 3.3V             |
| DATA      | GPIO 4           |
| GND       | GND              |

> Um resistor pull-up de **10kΩ entre DATA e VCC** pode ser necessário para maior estabilidade da leitura.

---

## 💡 LEDs (Controle de brilho via PWM)

### 📍 Conexões:

| LED       | Pino ESP32 | Observações                     |
|-----------|------------|----------------------------------|
| LED 1     | GPIO 32    | Controlado via PWM (canal 0)     |
| LED 2     | GPIO 33    | Definido mas **não usado** ainda |

> Lembre-se de usar resistores limitadores (220Ω ou 330Ω) em série com os LEDs.

---

## ☀️ LDR (Fotoresistor)

### 📍 Conexões:

| Pino       | Conecta ao       |
|------------|------------------|
| Um lado do LDR | 3.3V         |
| Outro lado do LDR | GPIO 36 (VP) + resistor 10kΩ para GND |

> Este é um divisor de tensão com o LDR e o resistor de 10kΩ. O sinal lido pelo ESP32 será no GPIO 36 (entrada analógica).

---

## ⚙️ Observações Gerais

- O ESP32 deve estar alimentado via USB com 5V.
- A função `analogReadResolution(12)` configura a resolução do ADC para 12 bits (0-4095).
- O valor `ldrMax` foi definido como `4095` para normalização.
- O display mostra `Temp`, `Hum` e `LDR` constantemente.
- Comandos podem ser enviados via monitor serial:
  - `SET_LED <valor>` de 0 a 100 para controlar o brilho.
  - `GET_LED`, `GET_LDR`, `GET_TEMP`, `GET_HUM`.

---

## 🛠️ Exemplo de Comando Serial

```text
SET_LED 50
GET_TEMP
GET_HUM
GET_LDR