/*
   Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
   Ported to Arduino ESP32 by pcbreflux
*/
//#define PERIPHERAL
//#define CENTRAL

//----PERIPHERAL---- "START"
#ifdef PERIPHERAL
#include "sys/time.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"
#include <M5StickC.h>
#include "BLE_setting_def.h"


#define GPIO_DEEP_SLEEP_DURATION     2  // sleep x seconds and then wake up
 RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory
 RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory
 
#ifdef __cplusplus
 extern "C" {
#endif
 
 //uint8_t g_phyFuns;
#ifdef __cplusplus
 }
#endif

 // See the following for generating UUIDs:
 // https://www.uuidgenerator.net/
 BLEAdvertising *pAdvertising;
 struct timeval now;
 
 /*
   Create a BLE server that will send periodic iBeacon frames.
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create advertising data
   3. Start advertising.
   4. wait
   5. Stop advertising.
   6. deep sleep
   
 */
 
 void setBeacon() { 
  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x5D00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor(MAJOR);
  oBeacon.setMinor(MINOR);
  oBeacon.setSignalPower(TX_POWER);
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  // Set Advertiing Type
  oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 0x0
  std::string strServiceData = "";
  
  strServiceData += (char)26;     // Len
  strServiceData += (char)0xFF;   // Type
  
  strServiceData += oBeacon.getData(); 
  oAdvertisementData.addData(strServiceData);
  
  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
  pAdvertising->setAdvertisementType(ADV_TYPE_NONCONN_IND);
 }
 
 void setup() {
  
  Serial.begin(115200);
  gettimeofday(&now, NULL);

  Serial.printf("start ESP32 %d\n",bootcount++);
  Serial.printf("deep sleep 5D (%lds since last reset, %lds since last boot)\n",now.tv_sec,now.tv_sec-last);

  last = now.tv_sec;
  
  // Create the BLE Device
  BLEDevice::init("");

  // Create the BLE Server
  // BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage

  pAdvertising = BLEDevice::getAdvertising();
  
  setBeacon();
   // Start advertising
  pAdvertising->start();
  Serial.println("Advertizing started...");
  delay(100);
  pAdvertising->stop();
  Serial.printf("enter deep sleep\n");
  esp_deep_sleep(1000000LL * GPIO_DEEP_SLEEP_DURATION);
  Serial.printf("in deep sleep\n");

 };

 void loop() {
 }

//----PERIPHERAL---- "End"
#else
//----CENTRAL---- "Start"
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
 //----CENTRAL---- "End"
#endif
 