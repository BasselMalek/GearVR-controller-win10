#pragma once

#include "WrapperHeader.h"

using namespace winrt::Windows;

struct KeyMappings {
  INPUT inputs[11];
  void initMappings(std::vector<uint8_t> scanKeys);
};

struct FusionSettings {
  float sensorGain;
  bool magnetEnable;
  bool rejectEnable;
  float rejectAccel;
  float rejectMagnet;
  float gyroSens[3];
  float gyroOffset[3];
  float accelSens[3];
  float accelOffset[3];
  float magnetOffset[3];
};

// Main class for read/write and I/O
class GearVRController {
public:
  // Possible controller modes.
  enum DEVICE_MODES { OFF, SENSORS, VR, CALIBRATE, KEEPAWAKE };

  GearVRController(uint64_t addr, std::vector<uint8_t> iniKeys, FusionSettings iniSettings);
  ~GearVRController();

  Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus
  writeCommand(GearVRController::DEVICE_MODES witeCommand);

  void startListener();
  void manualRead();
  void revokeListener();

  std::bitset<4> opFlags;

private:
  uint64_t MAC_address;
  DEVICE_MODES currentMode;
  winrt::event_token listenerToken;
  KeyMappings buttonMappings;
  FusionSettings fusionSettings;
  FusionAhrs fusionEngine;
  FusionOffset fusionOffsetParams;

  std::chrono::steady_clock::time_point lastStamp;
  // BluetoothLE GATT variables
  Devices::Bluetooth::BluetoothLEDevice deviceObject;
  Foundation::Collections::IVectorView<
      Devices::Bluetooth::GenericAttributeProfile::GattDeviceService>
      deviceServices;
  Devices::Bluetooth::GenericAttributeProfile::GattDeviceService commsService;
  Devices::Bluetooth::GenericAttributeProfile::GattDeviceService calibService;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic DATA_TX;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_RX;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic calibCharac;

  void mainEventHandler(
      Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
          &sender,
      Devices::Bluetooth::GenericAttributeProfile::
          GattValueChangedEventArgs const &args);
  void keyHandler(std::vector<bool> &keyStates, bool DPAD_MODE);
  bool dpadState(int xAxis, int yAxis, char direction);
  void touchHandler(int xAxis, int yAxis, int scaleFactor);
  FusionQuaternion fusionHandler(uint8_t rawBytes[18]);
  void fusionCursor(FusionEuler angles, bool refResetOne, bool refResetTwo);
  void fusionCursor(FusionQuaternion quat, bool refResetOne, bool refResetTwo);
  void DEBUG_PRINT_HEXDATAEVENT(uint8_t *buffer);
  void DEBUG_PRINT_UUID();
};