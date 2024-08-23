#pragma once

#include "WrapperHeader.h"

using namespace winrt::Windows;

struct AxisData {
  int axis_x;
  int axis_y;
};

class CurrentResolution {
public:
  int width;
  int height;
  CurrentResolution();
  ~CurrentResolution();
};

struct MiscExtraData {
  float timestamp;
  int temperature;
};

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
  void revokeListener();

private:
  uint64_t MAC_address;
  DEVICE_MODES currentMode;
  // BluetoothLE GATT variables
  Devices::Bluetooth::BluetoothLEDevice deviceObject;
  Foundation::Collections::IVectorView<
      Devices::Bluetooth::GenericAttributeProfile::GattDeviceService>
      deviceServices;
  Devices::Bluetooth::GenericAttributeProfile::GattDeviceService commsService;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic DATA_TX;
  Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_RX;

  void mainEventHandler(
      Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
          &sender,
      Devices::Bluetooth::GenericAttributeProfile::
          GattValueChangedEventArgs const &args);
  void keyHandler(std::vector<bool> &keyStates);
  void touchHandler(int xAxis, int yAxis);
  void fusionHandler();
};

//// Data processing class
// class ControllerData {
// public:
//   ControllerData(GearVRController &con_obj);
//   ~ControllerData();
//   AxisData returnScaledTouchpadOutput();
//   MiscExtraData returnMiscExtra();
//   std::bitset<6> returnButtons();
//   void fullRefresh(int xAxis, int yAxis);
//
// private:
//   GearVRController *current_controller;
//   std::vector<int> buffer_vector;
// };