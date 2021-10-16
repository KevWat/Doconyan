// Refered from below link
// http://blog.livedoor.jp/sce_info3-craft/archives/9717154.html
// https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_scan/BLE_scan.ino
// https://qiita.com/DeepSpawn/items/2799a894f80a79b40974

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include <M5StickC.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "env_set.h"      //SSID_NAME, SSID_PASS, GAS_WEBAP_ADR, CYC_TIME(unit msec)
#include "BLE_setting_def.h"

#include "HTTPSRedirect.h"

int scanTime = 10; // In seconds, Unit: second

//Global
char json[256]; // up to 256 words

// Redirect
const char* host = "script.google.com";
const int httpsPort = 443; // https port is 443

HTTPSRedirect* client = nullptr;

//  iBeacon Class
class iBeacon {
	private:
		String uuid;
		uint16_t major;
		uint16_t minor;
		int rssi;
	public:
		// Data picked up from BLEAdvertisedDevice
		bool createByAdvertisedDevice(BLEAdvertisedDevice advertisedDevice);
		String getUUID() {
			return uuid;
		}
		uint16_t getMajor() {
			return major;
		}
		uint16_t getMinor() {
			return minor;
		}
		int getRSSI() {
			return rssi;
		}
};

// Data picked up from BLEAdvertisedDevice
bool iBeacon::createByAdvertisedDevice(BLEAdvertisedDevice advertisedDevice) {
	char work [7];

	//Receive Data
	String hexString = (String) BLEUtils::buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().data(), advertisedDevice.getManufacturerData().length());
	//Classfiy iBeacon
	if (hexString.substring(0, 2).equals("5d")) {
		// add "-" on UUID
		uuid = hexString.substring(8, 16) + "-" + hexString.substring(16, 20) + "-" +
		       hexString.substring(20, 24) + "-" + hexString.substring(24, 28) + "-" +
		       hexString.substring(28, 40);
		("0x" + hexString.substring(40, 44)).toCharArray(work, 7); //16bit data copy as 8Byte data
		major = (uint16_t) atof(work); // string to double
		("0x" + hexString.substring(44, 48)).toCharArray(work, 7);
		minor = (uint16_t) atof(work);
		rssi = advertisedDevice.getRSSI();
		return true;
	}
	else {
		// not iBeacon
		return false;
	}
}

BLEScan* pBLEScan;

//https://ryjkmr.com/esp32-cds-google-spreadsheet/
void senddata() {
	static int error_count = 0;
	bool flag = false;// connection success or fail

	client = new HTTPSRedirect(httpsPort);
	client->setInsecure();
	client->setPrintResponseBody(true);
	client->setContentTypeHeader("application/json");
	Serial.print("Connecting to ");
	Serial.println(host);

	// Try to connect for a maximum of 5 times
	for (int i = 0; i < 5; i++) {
		int retval = client->connect(host, httpsPort);
		if (retval == 1) {
			flag = true;
			break;
		}
		else
			Serial.println("Connection failed. Retrying...");
		delay(500);
	}
	if (!flag) {
		delete client;
		Serial.print("Could not connect to server: ");
		return;
	}

	Serial.print("Successfully Connected to ");
	Serial.println(host);

	String payload = json;
	Serial.println(GAS_WEBAP_ADR);
	Serial.println(payload);
	if (client->POST(GAS_WEBAP_ADR, host, payload)) {
		Serial.println("Success! send data");
	}
	else {
		Serial.print("Error!");
	}
	delete client;
}

// Capture Advertised Data
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
		void onResult(BLEAdvertisedDevice advertisedDevice) {
			// When recived Adv data
			iBeacon ibcn;
			if (ibcn.createByAdvertisedDevice(advertisedDevice)) {
				char uuid[37];
				ibcn.getUUID().toCharArray(uuid, 37);
				Serial.printf("UUID: %s, Major: %d, Minor: %d, RSSI: %d\n", uuid, ibcn.getMajor(), ibcn.getMinor(), ibcn.getRSSI());
				int pri_data_mj = ibcn.getMajor();
				int pri_data_mi = ibcn.getMinor();
				int pri_data_RSSI = ibcn.getRSSI();
				int cnt_data_mj = MAJOR;//MAJOR;
				int cnt_data_mi = MINOR;//MINOR;
				sprintf(json, "{\"Central_Major\": %d, \"Central_Minor\": %d, \"Peripheral_Major\": %d, \"Peripheral_Minor\": %d, \"Peripheral_RSSI\": %d}", cnt_data_mj, cnt_data_mi, pri_data_mj, pri_data_mi, pri_data_RSSI);
				Serial.println(json);

				senddata();
			}
		}
};

//----- Main Part -----
void setup() {
	Serial.begin(115200);

	// Data Upload via M5 WiFi to GAS
	M5.begin();
	delay(100);
	Serial.print("Try to connect SSID ");
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

	Serial.print("Connected to ");
	Serial.println(SSID_NAME);
	Serial.println(WiFi.localIP());

	BLEDevice::init("");
	pBLEScan = BLEDevice::getScan(); // Create New Scan
	//コールバックを使うとWiFiとの共存がなぜか上手くいかない　btstop等でbtを止めてからWiFi通信は出来るが、その後bt再開が上手くいかない
	//pBLEScan -> setAdvertisedDeviceCallbacks (new MyAdvertisedDeviceCallbacks());
	pBLEScan -> setActiveScan(true); // Ture: Get result faster, with more power

	//pBLEScan->setInterval(100); //in Msec
	//pBLEScan->setWindow(99); // in mSec
}

void loop() {
	BLEScanResults foundDevices = pBLEScan -> start(scanTime);
	int found = foundDevices.getCount();
	if (found > 0) {
		Serial.printf("found %d\n",found);
		for (int i = 0; i < found; i++) {
			BLEAdvertisedDevice advertisedDevice = foundDevices.getDevice(i);
			iBeacon ibcn;
			if (ibcn.createByAdvertisedDevice(advertisedDevice)) {
				char uuid[37];
				ibcn.getUUID().toCharArray(uuid, 37);
				Serial.printf("UUID: %s, Major: %d, Minor: %d, RSSI: %d\n", uuid, ibcn.getMajor(), ibcn.getMinor(), ibcn.getRSSI());
				int pri_data_mj = ibcn.getMajor();
				int pri_data_mi = ibcn.getMinor();
				int pri_data_RSSI = ibcn.getRSSI();
				int cnt_data_mj = MAJOR;//MAJOR;
				int cnt_data_mi = MINOR;//MINOR;
				sprintf(json, "{\"Central_Major\": %d, \"Central_Minor\": %d, \"Peripheral_Major\": %d, \"Peripheral_Minor\": %d, \"Peripheral_RSSI\": %d}", cnt_data_mj, cnt_data_mi, pri_data_mj, pri_data_mi, pri_data_RSSI);
				Serial.println(json);
		    		senddata();
			}
		}
		pBLEScan->clearResults();
	}
	delay(3000); // msec
}
