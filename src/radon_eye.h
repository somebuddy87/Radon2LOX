#include <Arduino.h>
#include "BLEDevice.h"

static BLEUUID serviceUUID("00001523-1212-efde-1523-785feabcd123");
static BLEUUID charUUID("00001525-1212-efde-1523-785feabcd123");
static BLEUUID char24UUID("00001524-1212-efde-1523-785feabcd123");

class RadonEye {
public:
  RadonEye(String arg_address) : _ble_address(arg_address) { };

  boolean setup() {
    BLEDevice::init("");
    BLEAddress bleadr = BLEAddress(_ble_address.c_str());

    pClient  = BLEDevice::createClient();
    pClient->connect(bleadr, BLE_ADDR_TYPE_RANDOM);
    if(pClient->isConnected())
      return true;

    return false;
  }

  boolean update() {
    union { char c[4]; uint32_t b; float f; } radonval;
    union { char c[2]; uint16_t b; } pulsval;

    BLERemoteService* pRemoteService;
    BLERemoteCharacteristic* pRemoteCharacteristic;
    BLERemoteCharacteristic* p2RemoteCharacteristic;

    if(!pClient->isConnected()) {
      if(!setup() || !pClient->isConnected())
	return false;
    }

    // Obtain a reference to the service we are after in the remote BLE server.
    pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.printf("Failed to find our service UUID: %s\n", serviceUUID.toString().c_str());
      return false;
    }

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.printf("Failed to find our characteristic UUID: %s\n", charUUID.toString().c_str());
      return false;
    }

    // Obtain a reference to the characteristic to write to in the service of the remote BLE server.
    p2RemoteCharacteristic = pRemoteService->getCharacteristic(char24UUID);
    if (p2RemoteCharacteristic == nullptr) {
      Serial.printf("Failed to find our characteristic 1524 UUID: %s\n", charUUID.toString().c_str());
      return false;
    }

    //write 0x50 to request new data from sensor
    if(p2RemoteCharacteristic->canWrite()) {
      p2RemoteCharacteristic->writeValue(0x50);
      Serial.println("write Value to our characteristic 1524 with 0x50");
    }

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();

#if 0
      Serial.printf("result bytes: %02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X %02X%02X %02X%02X\n", value[0],value[1],value[2],value[3], value[4],value[5],value[6],value[7], value[8],value[9],value[10],value[11], value[12],value[13],value[14],value[15], value[16],value[17],value[18],value[19]);
      //on 0x50: 50100AD7 63406666 46400000 00000100 15000000
      //on 0x51: 510E0200 1D400000 0C4A0800 71595341 15000000 (Peakvalue12-15, Time since start 4-8)?
      //11tage 9h 37min 44s =985064 Peak 488
      //30s entspricht Byte[4] +1
#endif
      radonval.c[0] = value[2];
      radonval.c[1] = value[3];
      radonval.c[2] = value[4];
      radonval.c[3] = value[5];
      _radon_now = radonval.f;

      radonval.c[0] = value[6];
      radonval.c[1] = value[7];
      radonval.c[2] = value[8];
      radonval.c[3] = value[9];
      _radon_day = radonval.f;

      radonval.c[0] = value[10];
      radonval.c[1] = value[11];
      radonval.c[2] = value[12];
      radonval.c[3] = value[13];
      _radon_month = radonval.f;

      pulsval.c[0] = value[14];
      pulsval.c[1] = value[15];
      _pulse = pulsval.b;

      pulsval.c[0] = value[16];
      pulsval.c[1] = value[17];
      _pulse10 = pulsval.b;

      _last_updated = millis();

      Serial.printf("_Radon_Now %.2f Day %.2f Month %.2f _Pulsee %i _Pulse10min %i\n", _radon_now, _radon_day, _radon_month, _pulse, _pulse10);
      return true;
    }

    return false;
  }

  float radon_now() { return _radon_now; };
  float radon_day() { return _radon_day; };
  float radon_month() { return _radon_month; };
  int pulse_count() { return _pulse; };
  int pulse_count10() { return _pulse10; };
  unsigned long last_updated() { return _last_updated; };

private:
  String _ble_address;
  BLEClient*  pClient;

  float _radon_now = -1;
  float _radon_day = -1;
  float _radon_month = -1;
  int _pulse = 0;
  int _pulse10 = 0;

  unsigned long _last_updated = 0;
};