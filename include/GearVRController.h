#pragma once

#include "WrapperHeader.h"

using namespace winrt::Windows;

struct ControllerSettings {
  uint64_t macAddress;
  float fusionCursorSens;
  float touchCursorSens;
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
  std::vector<uint8_t> keys;
};

// Main class for read/write and I/O
class GearVRController {
public:
  // Utility
  static bool generateCleanIni(mINI::INIFile *ptrIni,
                               uint64_t controllerAddress);
  static uint64_t stoh(std::string src);
  static ControllerSettings loadFromIni();

  // Possible controller modes.
  enum DEVICE_MODES { OFF, SENSORS, VR, CALIBRATE, KEEPAWAKE };

  GearVRController(ControllerSettings iniSettings);
  ~GearVRController();

  // Settings adjustors.
  void initializeSettings(const ControllerSettings &iniSettings);
  void mapKeys(std::vector<uint8_t> scanKeys);

  Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus
  writeCommand(GearVRController::DEVICE_MODES witeCommand);

  void startListener();
  void revokeListener();
  void manualRead();

  void startOperation(std::bitset<4> opFlags);
  void pauseOperation();

private:
  uint64_t MAC_address;
  DEVICE_MODES currentMode;
  winrt::event_token listenerToken;

  ControllerSettings fusionSettings;
  std::vector<INPUT> buttonMappings;
  std::bitset<4> opFlags;
  FusionAhrs fusionEngine;
  FusionOffset fusionOffsetParams;
  std::chrono::steady_clock::time_point lastStamp;

  // BluetoothLE GATT variables.
  Devices::Bluetooth::BluetoothLEDevice deviceObject;
  Foundation::Collections::IVectorView<
      Devices::Bluetooth::GenericAttributeProfile::GattDeviceService>
      deviceServices;
  Devices::Bluetooth::GenericAttributeProfile::GattDeviceService commsService;
  Devices::Bluetooth::GenericAttributeProfile::GattDeviceService calibService;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic DATA_TX;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_RX;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic calibCharac;

  // Input Handlers.
  void mainEventHandler(
      Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
          &sender,
      Devices::Bluetooth::GenericAttributeProfile::
          GattValueChangedEventArgs const &args);
  void keyHandler(std::vector<bool> &keyStates, bool DPAD_MODE);
  bool dpadState(int xAxis, int yAxis, char direction);
  void touchHandler(int xAxis, int yAxis);
  FusionQuaternion fusionHandler(uint8_t rawBytes[18]);
  void fusionCursor(FusionEuler angles, bool refResetOne, bool refResetTwo);
  void fusionCursor(FusionQuaternion quat, bool refResetOne, bool refResetTwo);

  //Debugging Utilities. 
  void DEBUG_PRINT_HEXDATAEVENT(uint8_t *buffer);
  void DEBUG_PRINT_UUID();
};