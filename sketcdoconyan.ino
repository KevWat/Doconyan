/*
  Program for logging temperature and humidity on Google Spreadsheet.
 
 
MIT License
 
Copyright (c) 2018 @telomere0101
 
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
 
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
 
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
/* reffer from
http://www.telomere0101.site/archives/post-1008.html
*/

#include <M5StickC.h>
#include "DHT12.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <env_set.h>      //SSID_NAME, SSID_PASS, GAS_WEBAP_ADR, CYC_TIME(unit msec)

DHT12 dht12; //Preset scale CELSIUS and ID 0x5c.

//Global
char json[100];
float temp, humid;


//setup syquence
void setup(){
  // SetUp Network
    M5.begin();
    Wire.begin();
    //Serial Setting
    Serial.begin(115200);
    delay(100);

    Serial.print("Try to connect SSID");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println(SSID_NAME);
    WiFi.begin(SSID_NAME, SSID_PASS);

    //Set WiFi network
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(1000); // wait 1000msec and retry
    }
    
    Serial.print("Connected to");
    Serial.println(SSID_NAME);
    Serial.println(WiFi.localIP());
  // SetUp MultiThread
    //xTaskCreatePinnedToCore(タスクの関数名, "タスク名", スタックメモリサイズ, NULL, タスク優先順位, タスクハンドリングポインタ, Core ID)
    xTaskCreatePinnedToCore(task0, "Task_0", 4096, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(task1, "Task_1", 8192, NULL, 2, NULL, 1);
}

// MultiThread_0
void task0(void*arg){
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();
    while(1){
        // Measure from DHT12 sensor
        temp = (float)dht12.readTemperature();
        humid = (float)dht12.readHumidity();
        // Maske JSON format data to upload Cloud
        // To show data faster, use sprintf function
        sprintf(json, "{\"temp\": %2.1f, \"humid\": %2.1f }", temp, humid);
        //Show Temperature on Serian
        Serial.printf("Data %s \n", json);
        vTaskDelayUntil(&xLastWakeTime, CYC_TIME/portTICK_RATE_MS); // unit Tick
      }
}

// MultiThread_1
void task1(void*arg){
  portTickType xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();  
    while(1){
      //HTTP Client
      HTTPClient http;
      Serial.print("[HTTP] begin ...\n");
      // Configure upload server's URL
      // Insert Google Apps URL
      http.begin(GAS_WEBAP_ADR);
      // [Note] 本来はここにURLアクセスのエラー処理入れるべき
      Serial.print("[HTTP] POST...\n");
      // start connection and send data
      int httpCode = http.POST(json);
      // When httpCode was negative, it should be error
      if (httpCode > 0){
        // HTTP header will be sent and Server respons
        // and Handled 
        // start connection and send HTTP header
        // httpCode: 302 HTTP_CODE_FOUND, 200 HTTP_CODE_OK
        Serial.printf("[HTTP] GET.. code: %d\n", httpCode);
        // File exist on Server
          if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
            }
      } else {
        // HTTP error show as sting
        Serial.printf("[HTTP] GET ...failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
      vTaskDelayUntil(&xLastWakeTime, CYC_TIME/portTICK_RATE_MS); // unit Tick
    }
}

// Main Loop
void loop(){
   //Initial Set on LCD
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  M5.Lcd.setCursor(0,0);
  M5.Lcd.setTextSize(2);
  //Show Temperature on LCD
  M5.Lcd.printf("Temp:%2.1f C \r\nHumid:%2.1f %% \r\n",temp, humid);
  
  delay(CYC_TIME); // unit usec
}

