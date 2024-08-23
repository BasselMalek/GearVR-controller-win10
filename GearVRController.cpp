#include "GearVRController.h"
#include "WrapperHeader.h"

using namespace winrt::Windows;

INPUT KeyMappings::inputs[6] = {};

void KeyMappings::initMappings(uint8_t scanKeys[6]) {

  for (int i = 0; i < 6; i++) {
    KeyMappings::inputs[i].type = INPUT_KEYBOARD;
    // KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_SCANCODE;
    // KeyMappings::inputs[i].ki.wScan = scanKeys[i];
    KeyMappings::inputs[i].ki.wVk = scanKeys[i];
  }
}
/*
((((rawBuffer[54] & 0xF) << 6) + ((rawBuffer[55] & 0xFC) >> 2)) & 0x3FF) ->
touchpad X
((((rawBuffer[55] & 0x3) << 8) + ((rawBuffer[56] & 0xFF) >> 0)) & 0x3FF) ->
touchpad Y
*/

GearVRController::GearVRController(uint64_t address)
    : deviceObject(
          Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(
              address)
              .get()),
      deviceServices(deviceObject.GetGattServicesAsync().get().Services()),
      commsService(deviceServices.GetAt(5)),
      DATA_TX(
          commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              0)),
      COMMAND_RX(
          commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              1)),
      currentMode(DEVICE_MODES::OFF)

{
  GearVRController::MAC_address = address;
}

GearVRController::~GearVRController() {}

void GearVRController::startListener() {
  auto notifyStatus =
      this->DATA_TX
          .WriteClientCharacteristicConfigurationDescriptorAsync(
              Devices::Bluetooth::GenericAttributeProfile::
                  GattClientCharacteristicConfigurationDescriptorValue::Notify)
          .get();
  if (notifyStatus == Devices::Bluetooth::GenericAttributeProfile::
                          GattCommunicationStatus::Success) {
    std::cout << "Successfully subscribed to characteristic notifications.";
  } else {
    std::cout << "Notification subscription unsuccessful.";
  }
  auto activeListener =
      this->DATA_TX.ValueChanged({this, &GearVRController::mainEventHandler});
}

void GearVRController::mainEventHandler(
    Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
        &sender,
    Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs const
        &args) {
  auto rawBuffer = args.CharacteristicValue().data();
  std::vector<bool> states(
      {(bool)(rawBuffer[58] & (1 << 0)), (bool)(rawBuffer[58] & (1 << 1)),
       (bool)(rawBuffer[58] & (1 << 2)), (bool)(rawBuffer[58] & (1 << 3)),
       (bool)(rawBuffer[58] & (1 << 4)), (bool)(rawBuffer[58] & (1 << 5))});
  GearVRController::keyHandler(states);
}

//(bool)(rawBuffer[58] & (1 << 0)); // 0 -> trigger button
//(bool)(rawBuffer[58] & (1 << 1)); // 1 -> home button
//(bool)(rawBuffer[58] & (1 << 2)); // 2 -> back button
//(bool)(rawBuffer[58] & (1 << 3)); // 3 -> touchpad button
//(bool)(rawBuffer[58] & (1 << 4)); // 4 -> vol up button
//(bool)(rawBuffer[58] & (1 << 5)); // 5 -> vol down button
void GearVRController::keyHandler(std::vector<bool> &keyStates) {
  static std::vector<bool> prev(6, 0);
  for (int i = 0; i < 6; i++) {
    if (keyStates[i] && !prev[i]) {
      // KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_SCANCODE;
      KeyMappings::inputs[i].ki.dwFlags = 0;
      SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
    } else if (!keyStates[i] && prev[i]) {
      // KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_SCANCODE |
      // KEYEVENTF_KEYUP;
      KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
    }
  }
  prev = keyStates;
}

Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus
GearVRController::writeCommand(GearVRController::DEVICE_MODES writeCommand) {
  Storage::Streams::IBuffer send_packet;
  switch (writeCommand) {
  case GearVRController::DEVICE_MODES::OFF:
    send_packet =
        Security::Cryptography::CryptographicBuffer::DecodeFromHexString(
            L"0000");
    break;
  case GearVRController::DEVICE_MODES::SENSORS:
    send_packet =
        Security::Cryptography::CryptographicBuffer::DecodeFromHexString(
            L"0100");
    break;
  case GearVRController::DEVICE_MODES::VR:
    send_packet =
        Security::Cryptography::CryptographicBuffer::DecodeFromHexString(
            L"0800");
    break;
  case GearVRController::DEVICE_MODES::CALIBRATE:
    send_packet =
        Security::Cryptography::CryptographicBuffer::DecodeFromHexString(
            L"0300");
    break;
  case GearVRController::DEVICE_MODES::KEEPAWAKE:
    send_packet =
        Security::Cryptography::CryptographicBuffer::DecodeFromHexString(
            L"0400");
    break;
  default:
    break;
  }
  currentMode = writeCommand;
  return COMMAND_RX.WriteValueAsync(send_packet).get();
}

//// Return current screen resolution
// CurrentResolution::CurrentResolution() {
//   RECT desktop;
//   const HWND h_desktop = GetDesktopWindow();
//   GetWindowRect(h_desktop, &desktop);
//   width = desktop.right;
//   height = desktop.bottom;
// }
//
// CurrentResolution::~CurrentResolution() {}
//
// ControllerData::ControllerData(GearVRController &con_obj) {
//   GearVRController *temp_ptr = &con_obj;
//   ControllerData::current_controller = temp_ptr;
// }
//
// ControllerData::~ControllerData() { delete current_controller; }
//

// MiscExtraData ControllerData::returnMiscExtra() {
//   ControllerData::fullRefresh();
//   float temp_timestamp =
//       ((buffer_vector[0] + buffer_vector[1] + buffer_vector[2]) & 0xFFFFFFFF)
//       / static_cast<float>(1000) * 0.001;
//   int temp_temperature = buffer_vector[57];
//   MiscExtraData time;
//   time.temperature = temp_temperature;
//   time.timestamp = temp_timestamp;
//   return time;
// }
