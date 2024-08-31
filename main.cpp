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

int main() {
  mINI::INIFile configIniFile("config.ini");
  mINI::INIStructure configIni;
  if (!configIniFile.read(configIni)) {
    std::cout << generateCleanIni(&configIniFile, 0x2CBABA2FE1DC);
  }
  GearVRController ControllerObject =
      GearVRController(strtouhex(configIni["mac"]["address"]));
  std::vector<uint8_t> keys;
  for (auto &const keyValue : configIni.get("buttons")) {
    keys.push_back(strtouhex(configIni["buttons"][keyValue.first]));
  }
  KeyMappings::initMappings(keys);
  ControllerObject.writeCommand(GearVRController::VR);
  ControllerObject.writeCommand(GearVRController::SENSORS);
  ControllerObject.startListener();
  std::cin.get();
}