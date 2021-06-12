// GearVR-10.cpp : This file contains the 'main' function. Program execution begins and ends there.
//just wanted to say i hate MS' stupid projection, STOP USING ARBITRARY TYPES GODDAMN IT.
//TODO: "Switch to async and multithreaded worflow." (0)
//TODO: "Use device enumration instead of a direct MAC, this will solve a lot of the inconvinent stuff here." (1)

#include "WrapperHeader.h"

//TODO: "Create a modes enum" (2).


class GearVRcontroller
{
public:
	GearVRcontroller(uint64_t addr);
	~GearVRcontroller();

public:
	enum data_mode
	{
		raw = 0, formatted = 1
	};
	std::vector<int> return_reading(data_mode mode); //raw is a pretty bad implementation, returns a stoi which is useless outside of debugging.
	
private:
	uint64_t MAC_addr;
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> Services_list;
};

GearVRcontroller::GearVRcontroller(uint64_t addr)
{
	////Initializing address and device.
	MAC_addr = addr;
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(MAC_addr).get();

	//Initializing the controller's main service and its main chars (not async).
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult Service_r_obj = device.GetGattServicesAsync().get();
	Services_list = Service_r_obj.Services();
}

GearVRcontroller::~GearVRcontroller()
{
	//device.Close();
	//std::cout << "Connection with  " << winrt::to_string(device.Name()) << " closed" << std::endl;
}

std::vector<int> GearVRcontroller::return_reading(data_mode mode)
{
	//Initializing the main service and its characteristics
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService main_service = Services_list.GetAt(5);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult charac_r_obj = main_service.GetCharacteristicsAsync().get();
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> Charac_list = charac_r_obj.Characteristics();
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic INFO_CHAR = Charac_list.GetAt(0);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_CHAR = Charac_list.GetAt(1);

	//TODO: ->(2) "Make the write command a seperate function that uses the maps from (2)" (3).
    //Creating buffer and sending command.
	winrt::Windows::Storage::Streams::IBuffer send_packet = winrt::Windows::Security::Cryptography::CryptographicBuffer::DecodeFromHexString(L"0100");
	COMMAND_CHAR.WriteValueAsync(send_packet);

	auto read_result = INFO_CHAR.ReadValueAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
	auto read = read_result.Value();
	auto result = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(read);
	std::string raw_readable_result = winrt::to_string(result);
	if (mode == 1)
	{
		std::vector<int> result_vec;
		for (int i = 0; i < 120; i += 2)
		{
			std::string tempstr;
			tempstr.append(1, raw_readable_result[i]);
			tempstr.append(1, raw_readable_result[i + 1]);
			std::stringstream ss;
			ss << std::hex << tempstr;
			unsigned int x;
			ss >> x;
			result_vec.push_back(x);
		}
		return result_vec;
	}
	else
	{
		std::vector<int> result_vec;
		result_vec.push_back(std::stoi(raw_readable_result));
		return result_vec;
	}
}
int main()
{
	uint64_t address = 0x2CBABA2FE1DC;
	GearVRcontroller controller(address);
}