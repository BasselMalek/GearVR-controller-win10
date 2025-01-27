#include "GearVRController.h"
#include "WrapperHeader.h"

using namespace winrt::Windows;

uint64_t GearVRController::stoh(std::string src) {
  return (uint64_t)(std::strtoull(src.c_str(), nullptr, 0));
};
bool GearVRController::generateCleanIni(mINI::INIFile *ptrIni,
                                        uint64_t controllerAddress) {
  mINI::INIStructure SECTIONTEMPLATE;
  SECTIONTEMPLATE["mac"]["address"] = std::to_string(controllerAddress);
  SECTIONTEMPLATE["buttons"]["trigger"] = "0x01";
  SECTIONTEMPLATE["buttons"]["home"] = "0x5B";
  SECTIONTEMPLATE["buttons"]["back"] = "0x08";
  SECTIONTEMPLATE["buttons"]["touchpad-single"] = "0x02";
  SECTIONTEMPLATE["buttons"]["vol-up"] = "0xAF";
  SECTIONTEMPLATE["buttons"]["vol-down"] = "0xAE";
  SECTIONTEMPLATE["buttons"]["touchpad-center"] = "0x02";
  SECTIONTEMPLATE["buttons"]["touchpad-up"] = "0x26";
  SECTIONTEMPLATE["buttons"]["touchpad-down"] = "0x28";
  SECTIONTEMPLATE["buttons"]["touchpad-right"] = "0x27";
  SECTIONTEMPLATE["buttons"]["touchpad-left"] = "0x25";
  SECTIONTEMPLATE["cursors"]["fusion-sens"] = "5";
  SECTIONTEMPLATE["cursors"]["touchpad-sens"] = "5";
  SECTIONTEMPLATE["engine-params"]["sensor-gain"] = "0.5";
  SECTIONTEMPLATE["engine-params"]["magnet-enable"] = "1";
  SECTIONTEMPLATE["engine-params"]["reject-enable"] = "1";
  SECTIONTEMPLATE["engine-params"]["reject-accel"] = "0.5";
  SECTIONTEMPLATE["engine-params"]["reject-magnet"] = "0.5";
  SECTIONTEMPLATE["sensor-params"]["gyro-sens-x"] = "1";
  SECTIONTEMPLATE["sensor-params"]["gyro-sens-y"] = "1";
  SECTIONTEMPLATE["sensor-params"]["gyro-sens-z"] = "1";
  SECTIONTEMPLATE["sensor-params"]["gyro-offset-x"] = "0";
  SECTIONTEMPLATE["sensor-params"]["gyro-offset-y"] = "0";
  SECTIONTEMPLATE["sensor-params"]["gyro-offset-z"] = "0";
  SECTIONTEMPLATE["sensor-params"]["accel-sens-x"] = "1";
  SECTIONTEMPLATE["sensor-params"]["accel-sens-y"] = "1";
  SECTIONTEMPLATE["sensor-params"]["accel-sens-z"] = "1";
  SECTIONTEMPLATE["sensor-params"]["accel-offset-x"] = "0";
  SECTIONTEMPLATE["sensor-params"]["accel-offset-y"] = "0";
  SECTIONTEMPLATE["sensor-params"]["accel-offset-z"] = "0";
  SECTIONTEMPLATE["sensor-params"]["magnet-offset-x"] = "0";
  SECTIONTEMPLATE["sensor-params"]["magnet-offset-y"] = "0";
  SECTIONTEMPLATE["sensor-params"]["magnet-offset-z"] = "0";

  return ptrIni->generate(SECTIONTEMPLATE);
}
ControllerSettings GearVRController::loadFromIni() {

  mINI::INIFile configIniFile("config.ini");
  mINI::INIStructure configIni;
  if (!configIniFile.read(configIni)) {
    if (generateCleanIni(&configIniFile, GearVRController::stoh("0"))) {
      return ControllerSettings{.macAddress =
                                    1}; // very lazy error communication here
                                        // but i'm constrained to work with wx.
    } else {
      return ControllerSettings{.macAddress = 0};
    }
  } else {
    std::vector<uint8_t> keys;
    for (auto &const keyValue : configIni.get("buttons")) {
      keys.push_back(
          GearVRController::stoh(configIni["buttons"][keyValue.first]));
    }

    return ControllerSettings{
        .macAddress = GearVRController::stoh(configIni["mac"]["address"]),
        .fusionCursorSens = std::stof(configIni["cursors"]["fusion-sens"]),
        .touchCursorSens = std::stof(configIni["cursors"]["touchpad-sens"]),
        .sensorGain = std::stof(configIni["engine-params"]["sensor-gain"]),
        .magnetEnable = (configIni["engine-params"]["magnet-enable"] == "1"),
        .rejectEnable = (configIni["engine-params"]["reject-enable"] == "1"),
        .rejectAccel = std::stof(configIni["engine-params"]["reject-accel"]),
        .rejectMagnet = std::stof(configIni["engine-params"]["reject-magnet"]),
        .gyroSens = {std::stof(configIni["sensor-params"]["gyro-sens-x"]),
                     std::stof(configIni["sensor-params"]["gyro-sens-y"]),
                     std::stof(configIni["sensor-params"]["gyro-sens-z"])},
        .gyroOffset = {std::stof(configIni["sensor-params"]["gyro-offset-x"]),
                       std::stof(configIni["sensor-params"]["gyro-offset-y"]),
                       std::stof(configIni["sensor-params"]["gyro-offset-z"])},
        .accelSens = {std::stof(configIni["sensor-params"]["accel-sens-x"]),
                      std::stof(configIni["sensor-params"]["accel-sens-y"]),
                      std::stof(configIni["sensor-params"]["accel-sens-z"])},
        .accelOffset = {std::stof(configIni["sensor-params"]["accel-offset-x"]),
                        std::stof(configIni["sensor-params"]["accel-offset-y"]),
                        std::stof(
                            configIni["sensor-params"]["accel-offset-z"])},
        .magnetOffset =
            {std::stof(configIni["sensor-params"]["magnet-offset-x"]),
             std::stof(configIni["sensor-params"]["magnet-offset-y"]),
             std::stof(configIni["sensor-params"]["magnet-offset-z"])},
        .keys = keys,
    };
  }
};

GearVRController::GearVRController(ControllerSettings iniSettings)
    : deviceObject(
          Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(
              iniSettings.macAddress)
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
      lastStamp(std::chrono::steady_clock::now()), opFlags(std::bitset<4>(0)),
      fusionSettings(iniSettings) {

  this->MAC_address = this->fusionSettings.macAddress;
  this->buttonMappings.resize(15);
  initializeSettings(this->fusionSettings);
}
GearVRController::~GearVRController() {
  if (this->listenerToken) {
    this->revokeListener();
  }
}

void GearVRController::initializeSettings(
    const ControllerSettings &iniSettings) {
  this->mapKeys(this->fusionSettings.keys);

  FusionAhrsReset(&this->fusionEngine);
  FusionOffsetInitialise(&this->fusionOffsetParams, 69);
  FusionAhrsInitialise(&this->fusionEngine);

  const FusionAhrsSettings settings = {
      .convention = FusionConventionEnu,
      .gain = iniSettings.sensorGain,
      .gyroscopeRange = 1000.0f,
      .accelerationRejection = iniSettings.rejectAccel,
      .magneticRejection = iniSettings.rejectMagnet,
      .recoveryTriggerPeriod =
          iniSettings.rejectEnable
              ? static_cast<unsigned int>(5 * 69) /* 5 seconds */
              : 0,
  };

  FusionAhrsSetSettings(&this->fusionEngine, &settings);
}
void GearVRController::mapKeys(std::vector<uint8_t> scanKeys) {
  for (int i = 0; i < 11; i++) {
    if (scanKeys[i] == 0x01 || scanKeys[i] == 0x02) {
      this->buttonMappings[i].type = INPUT_MOUSE;
      this->buttonMappings[i].mi.dwFlags =
          scanKeys[i] == 0x01 ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_RIGHTDOWN;
    } else {
      this->buttonMappings[i].type = INPUT_KEYBOARD;
    }
    this->buttonMappings[i].ki.wVk = scanKeys[i];
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
void GearVRController::manualRead() {
  GearVRController::DEBUG_PRINT_HEXDATAEVENT(
      this->calibCharac.ReadValueAsync().get().Value().data());
}

void GearVRController::startOperation(std::bitset<4> opFlags) {
  this->opFlags = opFlags;
  this->writeCommand(GearVRController::VR);
  this->writeCommand(GearVRController::SENSORS);
  this->startListener();
}
void GearVRController::pauseOperation() {
  this->revokeListener();
  this->writeCommand(GearVRController::OFF);
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
    GearVRController::touchHandler(touchXAxis, touchYAxis);
  }
  if (GearVRController::opFlags[2]) {
    // Prevent fusion cursor re-center combination from executing presses.
    states[0] = states[0] && !states[3];
    states[3] = states[3] && !states[0];
    states[6] = states[6] && !states[0];
    GearVRController::keyHandler(states, this->opFlags[3]);
  }
}

bool GearVRController::dpadState(int xAxis, int yAxis, char direction) {
  xAxis = (xAxis - 157 == -157) ? 0 : xAxis - 157;
  yAxis = (-(yAxis - 157) == 157) ? 0 : -(yAxis - 157);
  double xRotated = (xAxis * cos(45) + yAxis * cos(45));
  double yRotated = (yAxis * cos(45) - xAxis * sin(45));
  switch (direction) {
  case 'C':
    // Adding a check to make sure it's not 0 introduces a dead zone at 0,0 but
    // prevents center action from activating if the touchpad is touched on the
    // rim.
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
      if (this->buttonMappings[i].type == INPUT_MOUSE) {
        SendInput(1, &this->buttonMappings[i], sizeof(INPUT));
      } else {
        this->buttonMappings[i].ki.dwFlags = 0;
        SendInput(1, &this->buttonMappings[i], sizeof(INPUT));
      }
    } else if (!keyStates[i] && prev[i]) {
      if (this->buttonMappings[i].type == INPUT_MOUSE) {
        // This is a very lazy implementation that prevents further mouse
        // mappings, should be changed later.
        this->buttonMappings[i].mi.dwFlags =
            this->buttonMappings[i].ki.wVk == 0x01 ? MOUSEEVENTF_LEFTUP
                                                   : MOUSEEVENTF_RIGHTUP;
        SendInput(1, &this->buttonMappings[i], sizeof(INPUT));
        this->buttonMappings[i].mi.dwFlags =
            this->buttonMappings[i].ki.wVk == 0x01 ? MOUSEEVENTF_LEFTDOWN
                                                   : MOUSEEVENTF_RIGHTDOWN;
      } else {
        this->buttonMappings[i].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &this->buttonMappings[i], sizeof(INPUT));
      }
    }
  }
  prev = keyStates;
}

void GearVRController::touchHandler(int xAxis, int yAxis) {
  static int xPrev = 0, yPrev = 0;
  xAxis = (xAxis * this->fusionSettings.touchCursorSens) -
          (157 * this->fusionSettings.touchCursorSens);
  yAxis = (yAxis * this->fusionSettings.touchCursorSens) -
          (157 * this->fusionSettings.touchCursorSens);
  static INPUT mouseInput = {};
  mouseInput.type = INPUT_MOUSE;
  mouseInput.mi.dx = (xAxis - xPrev);
  mouseInput.mi.dy = (yAxis - yPrev);
  if ((xAxis != (157 * -this->fusionSettings.touchCursorSens) &&
       yAxis != (157 * -this->fusionSettings.touchCursorSens) && xPrev &&
       yPrev) &&
      abs(xAxis - xPrev) > 1 && abs(yAxis - yPrev) > 1) {
    mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &mouseInput, sizeof(INPUT));
  } else {
    mouseInput.mi.dwFlags = 0;
  }
  xPrev = (xAxis == (157 * -this->fusionSettings.touchCursorSens)) ? 0 : xAxis;
  yPrev = (yAxis == (157 * -this->fusionSettings.touchCursorSens)) ? 0 : yAxis;
}

FusionQuaternion GearVRController::fusionHandler(uint8_t rawBytes[18]) {
  auto clock = std::chrono::steady_clock::now();
  static float scaledValues[9];
  for (int i = 0; i < (this->fusionSettings.magnetEnable ? 9 : 6); i++) {
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

  float timeDelta = (float)(clock - this->lastStamp).count() / 1000000000.0F;
  FusionVector accelerometer = {scaledValues[0], scaledValues[1],
                                scaledValues[2]};
  FusionVector gyroscope = {scaledValues[3], scaledValues[4], scaledValues[5]};

  gyroscope = FusionCalibrationInertial(
      gyroscope, {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
      FusionVector{
          this->fusionSettings.gyroSens[0],
          this->fusionSettings.gyroSens[1],
          this->fusionSettings.gyroSens[2],
      },
      FusionVector{
          this->fusionSettings.gyroOffset[0],
          this->fusionSettings.gyroOffset[1],
          this->fusionSettings.gyroOffset[2],
      });
  accelerometer = FusionCalibrationInertial(
      accelerometer, {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
      FusionVector{
          this->fusionSettings.accelSens[0],
          this->fusionSettings.accelSens[1],
          this->fusionSettings.accelSens[2],
      },
      FusionVector{
          this->fusionSettings.accelOffset[0],
          this->fusionSettings.accelOffset[1],
          this->fusionSettings.accelOffset[2],
      });
  gyroscope = FusionOffsetUpdate(&this->fusionOffsetParams, gyroscope);
  if (this->fusionSettings.magnetEnable) {
    FusionAhrsUpdate(
        &this->fusionEngine, gyroscope, accelerometer,
        FusionCalibrationMagnetic(
            FusionVector{scaledValues[6], scaledValues[7], scaledValues[8]},
            {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f},
            FusionVector{
                this->fusionSettings.magnetOffset[0],
                this->fusionSettings.magnetOffset[1],
                this->fusionSettings.magnetOffset[2],
            }),
        timeDelta);
  } else {
    FusionAhrsUpdateNoMagnetometer(&this->fusionEngine, gyroscope,
                                   accelerometer, timeDelta);
    this->lastStamp = clock;
  }

  return FusionAhrsGetQuaternion(&this->fusionEngine);
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
      yAxis = -(angles.angle.roll - pitchOffset) * 1456;
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

void GearVRController::fusionCursor(FusionQuaternion quat, bool refResetOne,
                                    bool refResetTwo) {
  static bool initLaunch = 1;
  static double currPitch = 0, currYaw = 0;
  static double prevPitch = 0, prevYaw = 0;
  static INPUT fusionInput = {};
  fusionInput.mi.dwFlags = MOUSEEVENTF_MOVE;
  if ((refResetOne && refResetTwo) || initLaunch) {
    fusionInput.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    fusionInput.mi.dx = 32767;
    fusionInput.mi.dy = 32767;
    SendInput(1, &fusionInput, sizeof(INPUT));
    initLaunch = 0;
  } else {
    currYaw = atan2(2 * (quat.element.w * quat.element.z +
                         quat.element.x * quat.element.y),
                    1 - 2 * (quat.element.y * quat.element.y +
                             quat.element.z * quat.element.z)) /
              M_PI;
    currPitch = std::atan2(2 * (quat.element.w * quat.element.x +
                                quat.element.y * quat.element.z),
                           1 - 2 * (quat.element.x * quat.element.x +
                                    quat.element.y * quat.element.y)) /
                M_PI;

    currYaw = std::round(1000 * currYaw);
    currPitch = std::round(1000 * currPitch);
    fusionInput.mi.dx =
        std::abs(currYaw - prevYaw) >= 1000
            ? 0
            : -(currYaw - prevYaw) * this->fusionSettings.fusionCursorSens;
    fusionInput.mi.dy =
        std::abs(currPitch - prevPitch) >= 1000
            ? 0
            : -(currPitch - prevPitch) * this->fusionSettings.fusionCursorSens;
    SendInput(1, &fusionInput, sizeof(INPUT));
    prevYaw = currYaw;
    prevPitch = currPitch;
  }
}

void GearVRController::DEBUG_PRINT_HEXDATAEVENT(uint8_t *buffer) {
  std::ostringstream convert;
  for (int a = 0; a < 59; a++) {
    convert << std::hex << (int)buffer[a];
  }
  std::string key_string = convert.str();
  std::cout << key_string << std::endl;
}
