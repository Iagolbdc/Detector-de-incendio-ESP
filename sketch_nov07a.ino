#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <SimpleDHT.h>

#define col 16
#define lin  2
#define ende  0x27

#define WIFI_NAME "INTELBRAS"
#define WIFI_PASS "12345678"

#define sensor_dht11 D9
#define sensor_dht22 D2
#define MQ2Pin A0

SimpleDHT11 dht11(sensor_dht11);
SimpleDHT22 dht22(sensor_dht22);

LiquidCrystal_I2C lcd(ende,col,lin);

ESP8266WebServer server(80);

byte t11 = 0;
byte h11 = 0;

float t22 = 0;
float h22 = 0;

float MQ2Value; 

unsigned long lastBlinkTime = 0;
bool lcdState = true;  

void setup() {
  Serial.begin(115200);

  initLCD();
  initWifi(WIFI_NAME, WIFI_PASS);

  server.on("/", handleRoot);
  server.begin();

  lcd.setCursor(0,0);
  lcd.print("LIGANDO MQ2");

  Serial.println("Web Server Iniciado");

  Serial.println("MQ2 warming up!");
  delay(10000);
}

void loop() {
  MDNS.update();
  MQ2Value = analogRead(MQ2Pin);
  Serial.println(MQ2Value);
  dhtSensors();
  server.handleClient();

  if (screenWarning() == false) {
      screenReadingSensor();
  }

  
}

void handleRoot(){
  String status = determineStatus();

  String json = "{";
  json += "\"Temperatura\":" + String(t22) + ",";
  json += "\"Umidade\":" + String(h22) + ",";
  json += "\"MQ2 Value\":" + String(MQ2Value) + ",";
  json += "\"Status\":\"" + String((t22 > 35.0 || MQ2Value > 400) ? "Warning" : "OK") + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void initWifi(String wname, String wpass){
  WiFi.mode(WIFI_STA);
  WiFi.begin(wname, wpass);
  Serial.println("Conectando a rede...");
  
  lcd.print("Conectando-se...");
  
  while(WiFi.status() != WL_CONNECTED){
      delay(500);
      Serial.println("."); 
  }

  if (!MDNS.begin("esp8266-device")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) { delay(1000); }
  }
  Serial.println("mDNS responder started");

  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  lcd.clear();
}

void initLCD(){
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void dhtSensors() {
  int err22 = dht22.read2(&t22, &h22, NULL);
  if (err22 == SimpleDHTErrSuccess) {
      Serial.print("Sample OK DHT22: ");
      Serial.print((int)t22); Serial.print(" *C, "); 
      Serial.print((int)h22); Serial.println(" H");
  } else {
      int err11 = dht11.read(&t11, &h11, NULL);
      if (err11 == SimpleDHTErrSuccess) {
          Serial.print("Sample OK DHT11: ");
          Serial.print((int)t11); Serial.print(" *C, "); 
          Serial.print((int)h11); Serial.println(" H");

          // Se falhar no DHT22, use valores do DHT11 como backup
          t22 = t11;
          h22 = h11;
      } else {
          Serial.println("Erro ao ler DHT11 e DHT22");
      }
  }
  delay(1000);
}

void screenReadingSensor(){
  lcd.setCursor(0,0);
  lcd.print("TEMPERATURA: ");
  lcd.print((int)t22);
  lcd.setCursor(0,1);
  lcd.print("FUMACA: OK      ");
}

bool screenWarning() {
  const float TEMPERATURA_LIMITE = 35.0; 
  const int MQ2_LIMITE = 400;            

  bool alertaTemperatura = (t22 > TEMPERATURA_LIMITE); 
  bool alertaFumaca = (MQ2Value > MQ2_LIMITE);

  if (alertaTemperatura || alertaFumaca) {
    lcd.clear();
    lcd.setCursor(0, 0);
    
    if (alertaTemperatura && alertaFumaca) {
      lcd.print("ALERTA!!!!!!!!");
      lcd.setCursor(0,1);
      lcd.print("ALTA TEMP/FUMACA");
    } else if (alertaTemperatura) {
      lcd.print("ALERTA!!!!!!!!!!");
      lcd.setCursor(0,1);
      lcd.print("ALTA TEMPERATURA");
    } else if (alertaFumaca) {
      lcd.print("ALERTA!!!!!!!");
      lcd.setCursor(0,1);
      lcd.print("ALTA FUMACA/GAS");
    }

    if (millis() - lastBlinkTime >= 100) {  
      lastBlinkTime = millis();
      lcdState = !lcdState;
      lcd.setBacklight(lcdState ? HIGH : LOW);
    }

    return true; 
  }
  lcd.setBacklight(HIGH);
  return false;
}

String determineStatus() {
  const float TEMPERATURA_LIMITE = 35.0; 
  const int MQ2_LIMITE = 400;

  bool alertaTemperatura = (t22 > TEMPERATURA_LIMITE);
  bool alertaFumaca = (MQ2Value > MQ2_LIMITE);

  if (alertaTemperatura || alertaFumaca) {
    return "Warning";
  }
  return "OK";
}
