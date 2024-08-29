#include "GearVRController.h"
#include "WrapperHeader.h"
#define SCALING_FACTOR 7

using namespace winrt::Windows;

INPUT KeyMappings::inputs[6] = {};

void KeyMappings::initMappings(uint8_t scanKeys[6]) {

  for (int i = 0; i < 6; i++) {
    KeyMappings::inputs[i].type = INPUT_KEYBOARD;
    KeyMappings::inputs[i].ki.wVk = scanKeys[i];
  }
}

GearVRController::GearVRController(uint64_t address)
    : deviceObject(
          Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(
              address)
              .get()),
      deviceServices(deviceObject.GetGattServicesAsync().get().Services()),
      commsService(deviceServices.GetAt(5)),
      calibService(deviceServices.GetAt(4)),
      calibCharac(
          calibService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              1)),
      DATA_TX(
          commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              0)),
      COMMAND_RX(
          commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              1)),
      currentMode(DEVICE_MODES::OFF),
      lastStamp(std::chrono::steady_clock::now()) {
  GearVRController::MAC_address = address;
  FusionOffsetInitialise(&this->fusionOffsetParams, 69);
  FusionAhrsInitialise(&this->fusionEngine);
  const FusionAhrsSettings settings = {
      .convention = FusionConventionNwu,
      .gain = 0.2f,
      .gyroscopeRange = 4000.0f,
      .accelerationRejection = 10.0f,
      .magneticRejection = 10.0f,
      .recoveryTriggerPeriod =
          static_cast<unsigned int>(5 * 69), /* 5 seconds */
  };
  FusionAhrsSetSettings(&this->fusionEngine, &settings);
}

GearVRController::~GearVRController() {}

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

void GearVRController::manualRead() {
  GearVRController::DEBUG_PRINT_HEXDATAEVENT(
      this->calibCharac.ReadValueAsync().get().Value().data());
}

void GearVRController::DEBUG_PRINT_HEXDATAEVENT(uint8_t *buffer) {
  std::ostringstream convert;
  for (int a = 0; a < 59; a++) {
    convert << std::hex << (int)buffer[a];
  }
  std::string key_string = convert.str();
  std::cout << key_string << std::endl;
}

void GearVRController::DEBUG_PRINT_UUID() {
  for (auto ser :
       this->calibService.GetCharacteristicsAsync().get().Characteristics()) {
    wchar_t struuid[39];
    auto result = StringFromGUID2(ser.Uuid(), struuid, 39);
    std::wcout << struuid << std::endl;
  }
}

void GearVRController::mainEventHandler(
    Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
        &sender,
    Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs const
        &args) {
  auto rawBuffer = args.CharacteristicValue().data();
  std::vector states = {
      (bool)(rawBuffer[58] & (1 << 0)), (bool)(rawBuffer[58] & (1 << 1)),
      (bool)(rawBuffer[58] & (1 << 2)), (bool)(rawBuffer[58] & (1 << 3)),
      (bool)(rawBuffer[58] & (1 << 4)), (bool)(rawBuffer[58] & (1 << 5))};
  // GearVRController::keyHandler(states);
  // GearVRController::touchHandler(
  //    ((((rawBuffer[54] & 0xF) << 6) + ((rawBuffer[55] & 0xFC) >> 2)) &
  //    0x3FF),
  //    ((((rawBuffer[55] & 0x3) << 8) + ((rawBuffer[56] & 0xFF) >> 0)) &
  //    0x3FF), SCALING_FACTOR);
  uint8_t accelBytes[18] = {
      rawBuffer[4],  rawBuffer[5],  rawBuffer[6],  rawBuffer[7],  rawBuffer[8],
      rawBuffer[9],  rawBuffer[10], rawBuffer[11], rawBuffer[12], rawBuffer[13],
      rawBuffer[14], rawBuffer[15], rawBuffer[32], rawBuffer[33], rawBuffer[34],
      rawBuffer[35], rawBuffer[36], rawBuffer[37]};
  auto fusionResult = GearVRController::fusionHandler(accelBytes);
  GearVRController::fusionCursor(fusionResult, states[0], states[3]);
}

void GearVRController::keyHandler(std::vector<bool> &keyStates) {
  static std::vector<bool> prev(6, 0);
  for (int i = 0; i < 6; i++) {
    if (keyStates[i] && !prev[i]) {
      KeyMappings::inputs[i].ki.dwFlags = 0;
      SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
    } else if (!keyStates[i] && prev[i]) {
      KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
      SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
    }
  }
  prev = keyStates;
}

void GearVRController::touchHandler(int xAxis, int yAxis, int scaleFactor) {
  static int xPrev = 0, yPrev = 0;
  xAxis = (xAxis * scaleFactor) - (157 * scaleFactor);
  yAxis = (yAxis * scaleFactor) - (157 * scaleFactor);
  static INPUT mouseInput = {};
  mouseInput.type = INPUT_MOUSE;
  mouseInput.mi.dx = (xAxis - xPrev);
  mouseInput.mi.dy = (yAxis - yPrev);
  if ((xAxis != (157 * -scaleFactor) && yAxis != (157 * -scaleFactor) &&
       xPrev && yPrev)) {
    mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &mouseInput, sizeof(INPUT));
  } else {
    mouseInput.mi.dwFlags = 0;
  }
  xPrev = (xAxis == (157 * -scaleFactor)) ? 0 : xAxis;
  yPrev = (yAxis == (157 * -scaleFactor)) ? 0 : yAxis;
}

FusionEuler GearVRController::fusionHandler(uint8_t rawBytes[18]) {
  auto clock = std::chrono::steady_clock::now();
  static float scaledValues[9] = {
      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  };
  for (int i = 0; i < 9; i++) {
    short int combinedFromBuffer =
        ((rawBytes[2 * i + 1] << 8) | rawBytes[2 * i]);
    if (i < 3) {
      scaledValues[i] = static_cast<float>(combinedFromBuffer * 10000.0f *
                                           9.80665f / 2048.0f);
      scaledValues[i] *= 0.00001f;
    } else if (i < 6) {
      scaledValues[i] = static_cast<float>(combinedFromBuffer * 10000.0f *
                                           0.017453292f / 14.285f);
      scaledValues[i] *= 0.0001f;
      scaledValues[i] *= 57.295779513082f;

    } else {
      scaledValues[i] = static_cast<float>(combinedFromBuffer * 0.06F);
    }
  }
  static const FusionMatrix gyroscopeMisalignment = {
      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  static const FusionVector gyroscopeSensitivity = {0.85f, 0.85f, 0.85f};
  static const FusionVector gyroscopeOffset = {2.0f, 2.0f, 2.0f};
  static const FusionMatrix accelerometerMisalignment = {
      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  static const FusionVector accelerometerSensitivity = {1.0f, 1.0f, 1.0f};
  static const FusionVector accelerometerOffset = {0.0f, 0.0f, 0.0f};
  static const FusionMatrix softIronMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                              0.0f, 0.0f, 0.0f, 1.0f};
  static const FusionVector hardIronOffset = {0.0f, 0.0f, 0.0f};

  float timeDelta = (float)(clock - this->lastStamp).count() / 1000000000.0F;
  FusionVector accelerometer = {scaledValues[0], scaledValues[1],
                                scaledValues[2]};
  FusionVector gyroscope = {scaledValues[3], scaledValues[4], scaledValues[5]};
  FusionVector mag = {scaledValues[6], scaledValues[7], scaledValues[8]};
  gyroscope = FusionCalibrationInertial(gyroscope, gyroscopeMisalignment,
                                        gyroscopeSensitivity, gyroscopeOffset);
  accelerometer =
      FusionCalibrationInertial(accelerometer, accelerometerMisalignment,
                                accelerometerSensitivity, accelerometerOffset);
  mag = FusionCalibrationMagnetic(mag, softIronMatrix, hardIronOffset);
  gyroscope = FusionOffsetUpdate(&this->fusionOffsetParams, gyroscope);
  FusionAhrsUpdate(&this->fusionEngine, gyroscope, accelerometer, mag,
                   timeDelta);
  this->lastStamp = clock;
  auto result =
      FusionQuaternionToEuler(FusionAhrsGetQuaternion(&this->fusionEngine));
  return result;
}

void GearVRController::fusionCursor(FusionEuler angles, bool refResetOne,
                                    bool refResetTwo) {
  static int initLaunch = 1;
  static int pitchOffset = 0, yawOffset = 0;
  static int prevPitch = 0, prevYaw = 0;
  static INPUT fusionInput = {};
  static INPUT clickInput = {0};
  fusionInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  static int xAxis = 32767, yAxis = 32767;
  if (refResetOne && !refResetTwo) {
    clickInput.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    SendInput(1, &clickInput, sizeof(INPUT));
  } else {
    clickInput.mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(1, &clickInput, sizeof(INPUT));
  }
  if ((refResetOne && refResetTwo) || initLaunch) {
    pitchOffset = angles.angle.roll;
    yawOffset = angles.angle.yaw;
    fusionInput.mi.dx = xAxis;
    fusionInput.mi.dy = yAxis;
    SendInput(1, &fusionInput, sizeof(INPUT));
    initLaunch = 0;
  } else {
    if ((angles.angle.roll - pitchOffset) < -60) {
      yAxis = 0;
    } else if ((angles.angle.roll - pitchOffset) > 60) {
      yAxis = 32767;
    } else {
      yAxis = (angles.angle.roll - prevPitch < 1)
                  ? prevPitch
                  : -(angles.angle.roll - pitchOffset) * 1092;
    }
    if ((angles.angle.yaw - yawOffset) < -80) {
      xAxis = 0;
    } else if ((angles.angle.yaw - yawOffset) > 80) {
      xAxis = 32767;
    } else {
      xAxis = (abs(angles.angle.yaw - prevYaw) < 3)
                  ? prevYaw
                  : -(angles.angle.yaw - yawOffset) * 728;
      ;
    }
    fusionInput.mi.dx = xAxis + 32767;
    fusionInput.mi.dy = yAxis + 32767;
    prevPitch = angles.angle.roll - pitchOffset;
    prevYaw = angles.angle.yaw - yawOffset;
    SendInput(1, &fusionInput, sizeof(INPUT));
  }
}

// MiscExtraData ControllerData::returnMiscExtra() {
//   ControllerData::fullRefresh();
//   float temp_timestamp =
//       ((buffer_vector[0] + buffer_vector[1] + buffer_vector[2]) &
//       0xFFFFFFFF) / static_cast<float>(1000) * 0.001;
//   int temp_temperature = buffer_vector[57];
//   MiscExtraData time;
//   time.temperature = temp_temperature;
//   time.timestamp = temp_timestamp;
//   return time;
// }
