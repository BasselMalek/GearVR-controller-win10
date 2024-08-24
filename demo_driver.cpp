/*
 * Demo app to test out changes in the code or to demo to other people!
 */
#include "GearVRController.h"
#include "WrapperHeader.h"

int main() {
  std::cout << "Welcome to the C++ implementation of the GearVR controller!\n";
  // uint8_t mp[6] = {0x001E, 0x0030, 0x002E, 0x0020, 0x0012, 0x0021};
  uint8_t mp[6] = {'A', 0x0030, 0x002E, 'F', 0xAF, 0xAE};
  KeyMappings::initMappings(mp);
  uint64_t address = 0x2CBABA2FE1DC;
  GearVRController Controller(address);
  auto vrStatus = Controller.writeCommand(GearVRController::VR);
  auto status = Controller.writeCommand(GearVRController::SENSORS);
  Controller.startListener();
  std::cin.get();
  return 0;
}