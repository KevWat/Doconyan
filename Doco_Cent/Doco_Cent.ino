
// Refered from below link 
// http://blog.livedoor.jp/sce_info3-craft/archives/9717154.html
// https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_scan/BLE_scan.ino

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <env_set.h>      //SSID_NAME, SSID_PASS, GAS_WEBAP_ADR, CYC_TIME(unit msec)
#include "BLE_setting_def.h"

int scanTime = 10; // In seconds, Unit: second

//Global
char json[256]; // up to 256 words

//  iBeacon Class
class iBeacon{
    private:
     String uuid;
     uint16_t major;
     uint16_t minor;
     int rssi;
    public:
     // Data picked up from BLEAdvertisedDevice
     bool createByAdvertisedDevice(BLEAdvertisedDevice advertisedDevice);
     String getUUID() {return uuid;}
     uint16_t getMajor(){ return major;}
     uint16_t getMinor(){ return minor;}
     int getRSSI(){ return rssi;}
};

// Data picked up from BLEAdvertisedDevice
bool iBeacon::createByAdvertisedDevice(BLEAdvertisedDevice advertisedDevice){
    char work [7];

    //Receive Data
    String hexString = (String) BLEUtils::buildHexData(nullptr,(uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
    //Classfiy iBeacon
    if (hexString.substring(0,2).equals("5d")) {
        // add "-" on UUID
        uuid = hexString.substring(8,16) + "-" + hexString.substring(16,20) + "-" +
               hexString.substring(20,24) + "-" + hexString.substring(24,28) + "-" +
               hexString.substring(28,40);
        ("0x" + hexString.substring(40,44)).toCharArray(work,7); //16bit data copy as 8Byte data
        major = (uint16_t) atof(work); // string to double
        ("0x" + hexString.substring(44,48)).toCharArray(work,7);
        minor = (uint16_t) atof(work);
        rssi = advertisedDevice.getRSSI();
        return true;
    }
        else{
        // not iBeacon
         return false;
        }
}


void senddata(){ //HTTP送信部分
     //BLEScanResults foundDevices = pBLEScan -> start(scanTime);
     //Send data to GAS Via WiFi
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
}

BLEScan* pBLEScan;

// Capture Advertised Data
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks{
    void onResult(BLEAdvertisedDevice advertisedDevice){
        // When recived Adv data
        iBeacon ibcn;
        if (ibcn.createByAdvertisedDevice(advertisedDevice)){
            char uuid[37];
            ibcn.getUUID().toCharArray(uuid, 37);
            Serial.printf("UUID: %s, Major: %d, Minor: %d, RSSI: %d\n", uuid, ibcn.getMajor(),ibcn.getMinor(), ibcn.getRSSI());
            int pri_data_mj = ibcn.getMajor();
            int pri_data_mi = ibcn.getMinor();
            int pri_data_RSSI = ibcn.getRSSI();
            int cnt_data_mj = 1000;//MAJOR;
            int cnt_data_mi = 1000;//MINOR;
            sprintf(json, "{\"Central_Major\": %d, \"Central_Minor\": %d, \"Peripheral_Major\": %d, \"Peripheral_Minor\": %d, \"Peripheral_RSSI\": %d}", cnt_data_mj,cnt_data_mi, pri_data_mj, pri_data_mi, pri_data_RSSI);
            senddata();
        }
    }
};


//----- Main Part -----
void setup(){
    Serial.begin(115200);
    
    // Data Upload via M5 WiFi to GAS
    M5.begin();
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
      
    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); // Create New Scan
    pBLEScan -> setAdvertisedDeviceCallbacks (new MyAdvertisedDeviceCallbacks());
    pBLEScan -> setActiveScan(true); // Ture: Get result faster, with more power
   
}

void loop(){
    BLEScanResults foundDevices = pBLEScan -> start(scanTime);
    Serial.println("Loop start");
    delay(3000); // msec 
   }
