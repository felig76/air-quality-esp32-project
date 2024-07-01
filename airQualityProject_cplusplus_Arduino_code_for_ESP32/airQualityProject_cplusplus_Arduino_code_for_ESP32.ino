// Librerías para la conexión WiFi
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "time.h"
// Librerías para el display
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Adafruit_SSD1306.h>
// Variables para la conexión wifi
const char* ssid     = "BA Escuela";
const char* password = "";
const char* serverName = "https://airqualityproject.000webhostapp.com/post-esp-data.php";
String apiKeyValue = "tPmAT5Ab3j7F9";
String sensorName = "MQ135";
// Variables para la obtención de fecha y hora
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -10800;
const int   daylightOffset_sec = -10800;
// Constantes para el display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // Definición del objeto del display
// Constantes de pines
const int PIN_BUZZER = 5;
const int PIN_PULSADOR = 23;
const int PIN_LED = 19;
#define PIN_MQ135 A7
// Variables del flujo y valores del programa
bool modoFacil = false;
int alarma = 0;
int acumuladorMediciones = 0;
int contadorMediciones = 0;
// Variables de marcas de tiempo para calcular lapsos de tiempo y delays
int lastValueTime = millis();
int lastPostTime = millis();
int lastPrintTime = millis();
int pressDelay = millis();
int ultimoPrendidoBuzzer = millis();
int delayPulsador = millis();
// Bitmap para imagen de calavera
const unsigned char bitmap_calavera [] PROGMEM = {
    0x00, 0x7f, 0x00, 0x00, 0x01, 0x80, 0xc0, 0x00, 0x02, 0x00, 0x20, 0x00, 0x04, 0x00, 0x10, 0x00, 
    0x00, 0x00, 0x10, 0x00, 0x08, 0x00, 0x18, 0x00, 0x0e, 0x00, 0x38, 0x00, 0x0e, 0x00, 0x38, 0x00, 
    0x0e, 0x00, 0x30, 0x00, 0x06, 0xe3, 0xb0, 0x00, 0x07, 0xe3, 0xe0, 0x00, 0x22, 0xe3, 0xa2, 0x00, 
    0x52, 0x49, 0x25, 0x00, 0x4e, 0x1c, 0x39, 0x00, 0x87, 0x9c, 0xe0, 0x80, 0x79, 0xc1, 0xcf, 0x00, 
    0x07, 0xc1, 0xf0, 0x00, 0x01, 0xff, 0xc0, 0x00, 0x01, 0xfe, 0xc0, 0x00, 0x1c, 0x80, 0x9c, 0x00, 
    0x21, 0xc1, 0xc2, 0x00, 0x26, 0x3e, 0x32, 0x00, 0x1c, 0x00, 0x1c, 0x00, 0x18, 0x00, 0x0c, 0x00
};
// Medición inicial
int medicion = map(((analogRead(PIN_MQ135) - 20) * 3.04), 0, 1023, -40, 125);

void setup() {
  // Puerto serie
  Serial.begin(115200);
  Serial.println("Iniciación de puerto serie.");
  // pinModes de alarma y pulsador
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_PULSADOR, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  // Begin de oled
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }else{
    Serial.println("Iniciación de display.");
  }
  // Begin de WiFi
  WiFi.begin(ssid, password);
  int intentosConexionWifi = 0;
  Serial.println("Se intentará una conexión wifi.");
  while((WiFi.status() != WL_CONNECTED) && (intentosConexionWifi<3)) { 
    Serial.print("Intentando conexión... (");
    Serial.print(intentosConexionWifi);
    Serial.println(")");
    delay(500);
    Serial.print(".");
    intentosConexionWifi += 1;
  }
  Serial.println("");
  Serial.print("Conectado a WiFi con la siguiente ip: ");
  Serial.println(WiFi.localIP());
  // Begin de server de fecha y hora
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop() {

  // Cálculo de la medición promedio cada 200 milisegundos
  acumuladorMediciones += map(((analogRead(PIN_MQ135) - 20) * 3.04), 0, 1023, -40, 125); // mq135_sensor.getCorrectedPPM(temperature, humidity); // analogRead(PIN_SENSOR_MQ135)
  contadorMediciones += 1;
  if ((millis() - lastValueTime) >= 200){
    medicion = acumuladorMediciones / contadorMediciones;
    contadorMediciones = 0;
    acumuladorMediciones = 0;
    lastValueTime = millis();
  }

  // Printeo en el display
  display.clearDisplay();
  display.setTextColor(WHITE);
  // En caso en el que el modo fácil esté activado
  if (modoFacil){
    // Case de alarmas para printear diferentes mensajes
    if (alarma == 2){
      display.setTextSize(2);
      display.setCursor(15, 15);
      display.println("Ventilar");
      display.println("urgente!");
    }else if (alarma == 0){
      display.setTextSize(2);
      display.setCursor(16, 5);
      display.println("Calidad");
      display.setCursor(34, 5);
      display.println("de aire");
      display.setCursor(52, 5);
      display.println("optima");
    }else{
      display.setTextSize(2);
      display.setCursor(15, 5);
      display.println("Calidad");
      display.println("de aire");
      display.println("media");
    }
    // En caso de que el modo fácil no esté acivado
  }else{

    // Se obtiene la fecha y hora
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      display.println("Failed to obtain time");
      return;
    }else{
      display.println(&timeinfo, "%H:%M:%S %B %d");
    }
    // Se printean varios elementos
    display.setTextSize(2);
    display.setCursor(20, 25);
    display.print(medicion); // Medición
    display.setTextSize(1);
    if (medicion >= 1000){
      display.setCursor(60, 31);
    }else{
      display.setCursor(50, 31);
    }
    display.print(" ppm"); // Texto de 'ppm' acomodado dependiendo de los dígitos de la medición
    display.drawRect(10, 12, 80, 40, SSD1306_WHITE); // Rectángulo estético para englobar los dos últimos elementos
    // Chequeo de alarmas para printear diferentes símbolos
    if (alarma == 2){
      display.drawBitmap(95, 20, bitmap_calavera, 25, 24, SSD1306_WHITE);
    }else if (alarma == 0){
      display.drawLine(100, 33, 106, 40, WHITE);
      display.drawLine(106, 40, 120, 23, WHITE);
    }else{
      display.drawCircle(110, 32, 12, SSD1306_WHITE);
      display.drawLine(110, 25, 110, 34, WHITE);
      display.drawCircle(110, 37, 1, SSD1306_WHITE);
    }
  }
  display.display(); // Se printean en el display todos los elementos guardados al mismo tiempo

  // Se detecta el estado del pulsador
  if (digitalRead(PIN_PULSADOR) == LOW && ((millis() - delayPulsador) >= 500)){
    modoFacil = !modoFacil;
    delayPulsador = millis();
  }

  // Código de activación y desactivación de alarma
  if ((medicion >= 1000) && alarma != 2){
    digitalWrite(PIN_LED, HIGH);
    tone(PIN_BUZZER, 150);
    alarma = 2;
  }else if ((medicion >= 800) && (medicion < 1000) && alarma != 1){
    digitalWrite(PIN_LED, HIGH);
    noTone(PIN_BUZZER);
    alarma = 1;
  }else if ((medicion < 800) && alarma != 0){
    digitalWrite(PIN_LED, LOW);
    noTone(PIN_BUZZER);
    alarma = 0;
  }

  //POST cada 20 segundos
  if ((millis() - lastPostTime) >= 20000){
    if(WiFi.status()== WL_CONNECTED){
      // Se crea el cliente y el begin del http
      WiFiClientSecure *client = new WiFiClientSecure;
      client->setInsecure();
      HTTPClient https;
      https.begin(*client, serverName);
      https.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Se formula y envía el request guardando el código de respuesta
      String httpRequestData = "api_key=" + apiKeyValue + "&sensor=" + sensorName + "&medicion=" + medicion + "";
      Serial.print("httpRequestData: ");
      Serial.println(httpRequestData);
      int httpResponseCode = https.POST(httpRequestData);
      // Se informa al usuario la respuesta del post
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
      }
      else {
        Serial.print("Código de error: ");
        Serial.println(httpResponseCode);
      }
      https.end();
    // Se resetea el delay entre cada post
    lastPostTime = millis();
    }
  }
}
