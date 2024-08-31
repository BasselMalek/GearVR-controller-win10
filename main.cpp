#include "GearVRController.h"
#include "WrapperHeader.h"

bool generateCleanIni(mINI::INIFile *ptrIni, uint64_t controllerAddress) {

  static mINI::INIStructure SECTIONTEMPLATE;
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
  return ptrIni->generate(SECTIONTEMPLATE);
}

uint64_t strtouhex(std::string start) {
  return (uint64_t)(std::strtoull(start.c_str(), nullptr, 0));
}
// 0-> Fusion on/off
// 1-> Touchpad on/off
// 2-> Buttons on/off
// 3-> Dpad mode on/off

int main() {
  std::cout << R"(
>>=============================================================<<
||                                                             ||
||                                                             ||
||  _________                  ___    _________                ||
||  __  ____/__________ _________ |  / /__  __ \               ||
||  _  / __ _  _ \  __ `/_  ___/_ | / /__  /_/ /               ||
||  / /_/ / /  __/ /_/ /_  /   __ |/ / _  _, _/                ||
||  \____/  \___/\__,_/ /_/    _____/  /_/ |_|                 ||
||                                                             ||
||  ________      _____            ________                    ||
||  ____  _/________  /_______________  __/_____ ___________   ||
||   __  / __  __ \  __/  _ \_  ___/_  /_ _  __ `/  ___/  _ \  ||
||  __/ /  _  / / / /_ /  __/  /   _  __/ / /_/ // /__ /  __/  ||
||  /___/  /_/ /_/\__/ \___//_/    /_/    \__,_/ \___/ \___/   ||
||                                                             ||
||                                                             ||
>>=============================================================<<

)" << std::endl;
  mINI::INIFile configIniFile("config.ini");
  mINI::INIStructure configIni;
  if (!configIniFile.read(configIni)) {
    std::string adrstr;
    std::cout << "Enter to your controller's MAC address prefixed with 0x, you "
                 "can obtain it from a "
                 "BLE explorer program (Bluetooth LE Lab is recommended)."
              << std::endl;

    std::cin >> adrstr;
    if (generateCleanIni(&configIniFile, strtouhex(adrstr))) {
      std::cout << "config.ini successfully created!\nYou can modify the "
                   "keybindings there according to "
                   "https://learn.microsoft.com/en-us/windows/win32/inputdev/"
                   "virtual-key-codes. Restart the app to use the controller.";
    } else {
      std::cout << "ini file creation failed. Try again.";
    }
  } else {
    GearVRController ControllerObject =
        GearVRController(strtouhex(configIni["mac"]["address"]));
    std::vector<uint8_t> keys;
    for (auto &const keyValue : configIni.get("buttons")) {
      keys.push_back(strtouhex(configIni["buttons"][keyValue.first]));
    }
    KeyMappings::initMappings(keys);
    ControllerObject.writeCommand(GearVRController::VR);
    ControllerObject.writeCommand(GearVRController::SENSORS);
    while (true) {
      int choice;
      ControllerObject.revokeListener();
      std::cout << "-----------------------------------------------------------"
                   "------------\n";
      std::cout << "1. Toggle Fusion.\t Status:" << ControllerObject.opFlags[0]
                << std::endl;
      std::cout << "2. Toggle Touchpad.\t Status:"
                << ControllerObject.opFlags[1] << std::endl;
      std::cout << "3. Toggle Buttons.\t Status:" << ControllerObject.opFlags[2]
                << std::endl;
      std::cout << "4. Toggle D-pad Mode.\t Status:"
                << ControllerObject.opFlags[3] << std::endl;
      std::cout << "5. Start controller." << std::endl;
      std::cout << "-----------------------------------------------------------"
                   "------------\n";
      std::cin >> choice;
      switch (choice) {
      case 1:
        ControllerObject.opFlags.flip(0);
        break;
      case 2:
        ControllerObject.opFlags.flip(1);
        break;
      case 3:
        ControllerObject.opFlags.flip(2);
        break;
      case 4:
        ControllerObject.opFlags.flip(3);
        break;
      case 5:
        ControllerObject.startListener();
        system("pause");
        break;
      default:
        break;
      }
    }
  }
}