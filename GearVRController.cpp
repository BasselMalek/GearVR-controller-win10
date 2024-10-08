#include "GearVRController.h"
#include "WrapperHeader.h"
#define SCALING_FACTOR 7

using namespace winrt::Windows;

INPUT KeyMappings::inputs[11] = {};

void KeyMappings::initMappings(std::vector<uint8_t> scanKeys) {
  for (int i = 0; i < 11; i++) {
    if (scanKeys[i] == 0x01 || scanKeys[i] == 0x02) {
      KeyMappings::inputs[i].type = INPUT_MOUSE;
      KeyMappings::inputs[i].mi.dwFlags =
          scanKeys[i] == 0x01 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    } else {
      KeyMappings::inputs[i].type = INPUT_KEYBOARD;
    }
    KeyMappings::inputs[i].ki.wVk = scanKeys[i];
  }
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
      DATA_TX(commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              0)),
      COMMAND_RX(
          commsService.GetCharacteristicsAsync().get().Characteristics().GetAt(
              1)),
      currentMode(DEVICE_MODES::OFF),
      lastStamp(std::chrono::steady_clock::now()), opFlags(std::bitset<4>(0)) {
  GearVRController::MAC_address = address;
  FusionOffsetInitialise(&this->fusionOffsetParams, 69);
  FusionAhrsInitialise(&this->fusionEngine);
  const FusionAhrsSettings settings = {
      .convention = FusionConventionNwu,
      .gain = 0.3f,
      .gyroscopeRange = 2000.0f,
      .accelerationRejection = 0,
      .magneticRejection = 0,
      .recoveryTriggerPeriod =
          static_cast<unsigned int>(5 * 69), /* 5 seconds */
  };
  FusionAhrsSetSettings(&this->fusionEngine, &settings);
}

GearVRController::~GearVRController() {
  if (this->listenerToken) {
    this->revokeListener();
  }
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
    std::cout << "Successfully subscribed to characteristic notifications.\n";
  } else {
    std::cout << "Notification subscription unsuccessful.\n";
  }
  this->listenerToken =
      this->DATA_TX.ValueChanged({this, &GearVRController::mainEventHandler});
}
void GearVRController::revokeListener() {
  if (this->listenerToken) {
    this->DATA_TX.ValueChanged(this->listenerToken);
    this->listenerToken.value = 0;
  }
}


void GearVRController::mainEventHandler(
    Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic const
        &sender,
    Devices::Bluetooth::GenericAttributeProfile::GattValueChangedEventArgs const
        &args) {
  auto rawBuffer = args.CharacteristicValue().data();
  int touchXAxis =
      ((((rawBuffer[54] & 0xF) << 6) + ((rawBuffer[55] & 0xFC) >> 2)) & 0x3FF);
  int touchYAxis =
      ((((rawBuffer[55] & 0x3) << 8) + ((rawBuffer[56] & 0xFF) >> 0)) & 0x3FF);
  std::vector states = {
      (bool)(rawBuffer[58] & (1 << 0)),
      (bool)(rawBuffer[58] & (1 << 1)),
      (bool)(rawBuffer[58] & (1 << 2)),
      (bool)(rawBuffer[58] & (1 << 3)),
      (bool)(rawBuffer[58] & (1 << 4)),
      (bool)(rawBuffer[58] & (1 << 5)),
      ((bool)(rawBuffer[58] & (1 << 3)) &&
       GearVRController::dpadState(touchXAxis, touchYAxis, 'C')),
      ((bool)(rawBuffer[58] & (1 << 3)) &&
       GearVRController::dpadState(touchXAxis, touchYAxis, 'U')),
      ((bool)(rawBuffer[58] & (1 << 3)) &&
       GearVRController::dpadState(touchXAxis, touchYAxis, 'D')),
      ((bool)(rawBuffer[58] & (1 << 3)) &&
       GearVRController::dpadState(touchXAxis, touchYAxis, 'R')),
      ((bool)(rawBuffer[58] & (1 << 3)) &&
       GearVRController::dpadState(touchXAxis, touchYAxis, 'L'))};
  uint8_t accelBytes[18] = {
      rawBuffer[4],  rawBuffer[5],  rawBuffer[6],  rawBuffer[7],  rawBuffer[8],
      rawBuffer[9],  rawBuffer[10], rawBuffer[11], rawBuffer[12], rawBuffer[13],
      rawBuffer[14], rawBuffer[15], rawBuffer[32], rawBuffer[33], rawBuffer[34],
      rawBuffer[35], rawBuffer[36], rawBuffer[37]};
  auto fusionResult = GearVRController::fusionHandler(accelBytes);
  if (GearVRController::opFlags[0]) {
    GearVRController::fusionCursor(fusionResult, states[0], states[3]);
  }
  if (GearVRController::opFlags[1]) {
    GearVRController::touchHandler(touchXAxis, touchYAxis, SCALING_FACTOR);
  }
  if (GearVRController::opFlags[2]) {
    // Prevent fusion cursor re-center combination from executing presses.
    states[0] = states[0] && !states[3];
    states[3] = states[3] && !states[0];
    states[6] = states[6] && !states[0];
    GearVRController::keyHandler(states, this->opFlags[3]);
  }
}

void GearVRController::keyHandler(std::vector<bool> &keyStates,
                                  bool DPAD_MODE) {
  static std::vector<bool> prev(11, 0);
  int it = DPAD_MODE ? 11 : 6;
  for (int i = 0; i < it; i++) {
    if (DPAD_MODE && i == 3) {
      // skip touchpad action when in d-pad  mode.
      continue;
    }
    if (keyStates[i] && !prev[i]) {
      if (KeyMappings::inputs[i].type == INPUT_MOUSE) {
        SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
      } else {
        KeyMappings::inputs[i].ki.dwFlags = 0;
        SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
      }
    } else if (!keyStates[i] && prev[i]) {
      if (KeyMappings::inputs[i].type == INPUT_MOUSE) {
        // This is a very lazy implementation that prevents further mouse
        // mappings, should be changed later.
        KeyMappings::inputs[i].mi.dwFlags =
            KeyMappings::inputs[i].ki.wVk == 0x01 ? MOUSEEVENTF_LEFTUP
                                                  : MOUSEEVENTF_RIGHTUP;
        SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
        KeyMappings::inputs[i].mi.dwFlags =
            KeyMappings::inputs[i].ki.wVk == 0x01 ? MOUSEEVENTF_LEFTDOWN
                                                  : MOUSEEVENTF_RIGHTDOWN;
      } else {
        KeyMappings::inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &KeyMappings::inputs[i], sizeof(INPUT));
      }
    }
  }
  prev = keyStates;
}

bool GearVRController::dpadState(int xAxis, int yAxis, char direction) {
  xAxis = (xAxis - 157 == -157) ? 0 : xAxis - 157;
  yAxis = (-(yAxis - 157) == 157) ? 0 : -(yAxis - 157);
  int xRotated = (xAxis * cos(45) + yAxis * cos(45));
  int yRotated = (yAxis * cos(45) - xAxis * sin(45));
  switch (direction) {
  case 'C':
     //Adding a check to make sure it's not 0 introduces a dead zone at 0,0 but prevents center action from activating if the touchpad is touched on the rim.
    return (xAxis > -52 && xAxis < 52 && xAxis) &&
           (yAxis > -52 && yAxis < 52 && yAxis);
    break;
  case 'U':
    return (xRotated > 0 && yRotated > 0) &&
           !((xAxis > -52 && xAxis < 52 && xAxis) &&
             (yAxis > -52 && yAxis < 52 && yAxis));
    break;
  case 'R':
    return (xRotated > 0 && yRotated < 0) &&
           !((xAxis > -52 && xAxis < 52 && xAxis) &&
             (yAxis > -52 && yAxis < 52 && yAxis));
    break;
  case 'L':
    return (xRotated < 0 && yRotated > 0) &&
           !((xAxis > -52 && xAxis < 52 && xAxis) &&
             (yAxis > -52 && yAxis < 52 && yAxis));
    break;
  case 'D':
    return (xRotated < 0 && yRotated < 0) &&
           !((xAxis > -52 && xAxis < 52 && xAxis) &&
             (yAxis > -52 && yAxis < 52 && yAxis));
    break;
  default:
    return false;
    break;
  }
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
       xPrev && yPrev)&& abs(xAxis-xPrev)>1 && abs(yAxis-yPrev)>1) {
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
  static const FusionVector gyroscopeSensitivity = {0.35f, 0.35f, 0.35f};
  static const FusionVector gyroscopeOffset = {0.01f, 0.28f, 0.1f};
  static const FusionMatrix accelerometerMisalignment = {
      1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
  static const FusionVector accelerometerSensitivity = {0.95f, 1.0f, 1.0f};
  static const FusionVector accelerometerOffset = {0.00861f, 0.251564,
                                                   0.991599f};
  static const FusionMatrix softIronMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                              0.0f, 0.0f, 0.0f, 1.0f};
  static const FusionVector hardIronOffset = {100.0f, 20.0f, 15.700000f};

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

  //printf("R:%0.00f, P:%0.00f, Y:%0.00f\n", result.angle.pitch, result.angle.roll,
  //       result.angle.yaw);
  return result;
}

void GearVRController::fusionCursor(FusionEuler angles, bool refResetOne,
                                    bool refResetTwo) {
  static int initLaunch = 1;
  static float pitchOffset = 0, yawOffset = 0;
  static float prevPitch = 0, prevYaw = 0;
  static INPUT fusionInput = {};
  fusionInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
  static int xAxis = 32767, yAxis = 32767;
  if ((refResetOne && refResetTwo) || initLaunch) {
    pitchOffset = angles.angle.roll;
    yawOffset = angles.angle.pitch;
    fusionInput.mi.dx = xAxis;
    fusionInput.mi.dy = yAxis;
    SendInput(1, &fusionInput, sizeof(INPUT));
    initLaunch = 0;
  } else {
    angles.angle.roll = (abs(prevPitch - angles.angle.roll) > 0.2)
                            ? angles.angle.roll
                            : prevPitch;
    angles.angle.pitch = (abs(prevYaw - angles.angle.pitch) > 0.2)
                            ? angles.angle.pitch
                            : prevYaw;
    if ((angles.angle.roll - pitchOffset) < -45) {
      yAxis = 0;
    } else if ((angles.angle.roll - pitchOffset) > 45) {
      yAxis = 32767;
    } else {
      yAxis =  -(angles.angle.roll - pitchOffset) * 1456;
    }
    if ((angles.angle.pitch - yawOffset) < -45) {
      xAxis = 0;
    } else if ((angles.angle.pitch - yawOffset) > 45) {
      xAxis = 32767;
    } else {
      xAxis = (angles.angle.pitch - yawOffset) * 1456;
    }
    fusionInput.mi.dx = xAxis + 32767;
    fusionInput.mi.dy = yAxis + 32767;
    prevPitch = angles.angle.roll;
    prevYaw = angles.angle.pitch;
    SendInput(1, &fusionInput, sizeof(INPUT));
  }
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
