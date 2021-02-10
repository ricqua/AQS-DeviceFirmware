//https://www.youtube.com/watch?v=okNECYf2xlY

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include "DHT.h"
#define DHTTYPE DHT22
const int DHTPin = 2;
DHT dht(DHTPin, DHTTYPE);
const char* ssid = "srkim27";
const char* password = "jj121212!!";

//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;

const unsigned long sendDataInterval = 60000;
unsigned long previousSendData = 0;


WiFiClientSecure client;

String GAS_ID = "AKfycbxXNtNvqHGIlED1GuAKzC-el5YlNkmfBddmfv_1VIEM6ZVUJgEHetB_";

//LCD setup
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
const unsigned long lcdInterval = 1000;
unsigned long lcdPreviousTime = millis();

//Dust sensor
#include <SoftwareSerial.h>
SoftwareSerial dustSensor(D7, 0);     //Dust sensor pin
int pm1 = 0;
int pm2_5 = 0;
int pm10 = 0;

//Serial Setup
const unsigned long serialInterval = 5000;
unsigned long serialPreviousTime = millis();

//============================================================================== void setup
void setup() {
  Serial.begin(9600);
  delay(500);
  dht.begin();
  delay(500);

  dustSensor.begin(9600);

  WiFi.begin(ssid, password);
  Serial.println("");

  client.setInsecure();

  //Loading screen
  lcd.begin();
  lcd.backlight();
  Serial.println();
  Serial.println("Booting device");
  lcd.setCursor(0, 0);
  lcd.print("AQS - Device 2  ");
  delay(50);
  lcd.setCursor(0, 1);
  lcd.print("By: Kim HyunWook");
  delay(2000);
  lcd.clear();
}


//============================================================================== void loop
void loop() {
  int h = dht.readHumidity();
  float t = dht.readTemperature();
  String Temp = "Temperature : " + String(t) + " °C";
  String Humi = "Humidity : " + String(h) + " %";
  //  Serial.println(Temp);
  //  Serial.println(Humi);

  // Dust sensor check
  int index = 0;
  char value;
  char previousValue;

  while (dustSensor.available()) {
    value = dustSensor.read();
    if ((index == 0 && value != 0x42) || (index == 1 && value != 0x4d)) {
      //      Serial.println("Cannot find the dust sensor");
      break;
    }

    if (index == 4 || index == 6 || index == 8 || index == 10 || index == 12 || index == 14) {
      previousValue = value;
    }
    else if (index == 5) {
      pm1 = 256 * previousValue + value;
    }
    else if (index == 7) {
      pm2_5 = 256 * previousValue + value;
    }
    else if (index == 9) {
      pm10 = 256 * previousValue + value;
    } else if (index > 15) {
      break;
    }
    index++;
  }
  while (dustSensor.available()) dustSensor.read();

  //=============================  Serial Monitor ======================================
    if (millis() - serialPreviousTime > serialInterval) {
      Serial.print(millis());
      Serial.print("    ");
      Serial.print(t);
      Serial.print("ºC    ");
      Serial.print(h);
      Serial.print("%     ");
  
      Serial.print("PM1:  ");
      Serial.print(pm1);
      Serial.print("    ");
  
      Serial.print("PM2.5:  ");
      Serial.print(pm2_5);
      Serial.print("     ");
  
      Serial.print("PM10:  ");
      Serial.print(pm10);
      Serial.print("     ");
  
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Wifi Connected");
      } else {
        Serial.print("Wifi Not Connected");
      }
  
      Serial.println();
      serialPreviousTime = millis();
    }

  //=============================  LCD Printing ======================================
  if (millis() - lcdPreviousTime > lcdInterval) {

    lcd.setCursor(0, 0);
    lcd.print(t, 1);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print(h, 1);
    lcd.print("%");

    lcd.setCursor(6, 1);
    lcd.print(pm1);
    lcd.print("   ");

    lcd.setCursor(9, 0);
    lcd.print(pm2_5);
    lcd.print("   ");

    lcd.setCursor(12, 1);
    lcd.print(pm10);
    lcd.print("   ");

    if (WiFi.status() == WL_CONNECTED) {
      lcd.setCursor(15, 0);
      lcd.print("W");
    }
    else {
      lcd.setCursor(15, 0);
      lcd.print("*");
    }

    lcdPreviousTime = millis();
  }

  if (millis() - previousSendData > sendDataInterval) {
    sendData(t, h, pm1, pm2_5, pm10);
    previousSendData = millis();
  }


}



//============================================================================== void sendData
// Subroutine for sending data to Google Sheets
void sendData(float tem, int hum, int pm1, int pm2_5, int pm10) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);

  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  //----------------------------------------Processing data and sending data
  String string_temperature =  String(tem);
  // String string_temperature =  String(tem, DEC);
  String string_humidity =  String(hum, DEC);
  String string_pm1 =  String(pm1, DEC);
  String string_pm2_5 =  String(pm2_5, DEC);
  String string_pm10 =  String(pm10, DEC);
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature + "&humidity=" + string_humidity + "&pm1=" + string_pm1 + "&pm2_5=" + string_pm2_5 + "&pm10=" + string_pm10;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
}
