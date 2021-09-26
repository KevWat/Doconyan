# Doconyan
どこニャン

M5Stick-C用のArduinoソース<br>Google App Script(GAS)にデータをupload<br>GAS側でSpreadSheetに展開<br>WiFiのSSIDやPASSWORD, GASのWeb Applicationの情報は.zipでライブラリとして外部に配置(env_set.zip)<br>

[rev 0.1.2]<bar>
# Build: Central(Doco_Cent), Peripheral(Doco_Peri) 
#### CENTRAL's Specification<bar>
##### Data Flow
1. Receive Data from "Specific UUID's"  PERIPHERAL<bar>
2. Stored: RSSI, Major, Minor data from PERIPHERAL.
Named peri_data<bar>
3. Send: peri_data and this CENTRAL's Major, Minor data<bar>
Named centr_data<bar>
 to GAS via WiFi
4. Data format:<bar>
centr_data_Mj, centr_data_Mi,peri_data_Mj, peri_data_Mi, peri_data_RSSI<bar>
 
- Data Transfer Cycle depends on BLE Scan cycle  

#### Central Proceessing Flow<bar>
* Initial setting(M5,WiFi,BLE) on setup()
* BLE Scan on loop()
    * Call BLE scan callback on iBeacon
    * Once received Scan data as json, send Http

#### Folder Information<bar>
* Doco_Cent: BLE Central<bar>
* Doco_Peri: BLE Peripheral<bar>


[rev 0.1.1]<br>
1. Defined: PERIPHERAL / CENTRAL mode
2. Broadcast own Beacon as peripheral
3. CENTRAL mode does not work as BLE CENTRAL
3. Added BLE_setting_defh.h to configure BLE parameters

[rev 0.1]<br>
First design: To study the method of M5data(Temp, Humid) upload to GAS. <bar>
 

