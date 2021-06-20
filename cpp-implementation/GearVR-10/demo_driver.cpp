/*
* Demo app to test out changes in the code or to demo to other people!
*/
#include "WrapperHeader.h"
#include "GearVRController.h"

int main()
{
	std::vector<std::string> buttons = { "Trigger button", "Home button", "Back button", "Touchpad button", "Vol Up", "Vol Down" };
	std::cout << "Welcome to the C++ implementation of the GearVR controller!" << "\n";
	uint64_t address = 0x2CBABA2FE1DC;
	GearVRController Controller(address);
	ControllerData data_resolver(Controller);
	while (true)
	{
		std::cout << "Please choose an option: \n 1- Output raw hex string reading \n 2- Output formatted reading \n";
		int option;
		std::cin >> option;
		switch (option)
		{
		default:
			break;
		case 1:
			Controller.writeCommand();
			for (int i = 0; i < 100; i++)
			{
				std::cout << "Raw reading: " << Controller.returnRawReading() << "\n";
			}
			break;
		case 2:
			Controller.writeCommand();
			for (int i = 0; i < 100; i++)
			{
				auto touch = data_resolver.returnScaledTouchpadOutput();
				auto boool_list = data_resolver.returnButtons();
				std::cout << "Touchpad: (" << touch.axis_x << ", " << touch.axis_y << ")\n";
				for (int i = 0; i < boool_list.size(); i++)
				{
					std::cout << buttons[i] << ": " << boool_list[i] << "\n";
				}
				std::cout << std::endl;
			}
			break;
		}
		std::cout << "Exit or repeat (Y/N)? \n";
		char decide;
		std::cin >> decide;
		if (decide == 'Y' || decide == 'y')
		{
			exit(0);
		}
		else
		{
			continue;
		}
	}
	return 0;
}