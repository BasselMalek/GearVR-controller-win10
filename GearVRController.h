#pragma once

#include "WrapperHeader.h"

using namespace winrt::Windows;

struct KeyMappings {
  static INPUT inputs[6];
  static void initMappings(uint8_t scanKeys[6]);
};

// Main class for read/write and I/O
class GearVRController {
public:
  // Possible controller modes.
  enum DEVICE_MODES { OFF, SENSORS, VR, CALIBRATE, KEEPAWAKE };

  GearVRController(uint64_t addr);
  ~GearVRController();

  Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus
  writeCommand(GearVRController::DEVICE_MODES witeCommand);

  void startListener();
  void manualRead();
  void revokeListener();

private:
  uint64_t MAC_address;
  DEVICE_MODES currentMode;
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
  void keyHandler(std::vector<bool> &keyStates);
  void touchHandler(int xAxis, int yAxis, int scaleFactor);
  FusionEuler fusionHandler(uint8_t rawBytes[18]);
  void fusionCursor(FusionEuler angles, bool refResetOne, bool refResetTwo);
  void DEBUG_PRINT_HEXDATAEVENT(uint8_t *buffer);
  void DEBUG_PRINT_UUID();
};