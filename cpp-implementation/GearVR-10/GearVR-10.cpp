// GearVR-10.cpp : This file contains the 'main' function. Program execution begins and ends there.
// 
// 
//TODO: "Use device enumration instead of a direct MAC, this will solve a lot of the inconvinent stuff here." (1)

#include "WrapperHeader.h"

//TODO: "Create a modes map" (2).

class GearVRcontroller
{
public:
	GearVRcontroller(uint64_t addr);
	~GearVRcontroller();
	void Start_receive();
private:
	winrt::guid const IOGUUID = winrt::guid("4F63756C-7573-2054-6872-65656D6F7465");
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

void GearVRcontroller::Start_receive()
{
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService main_service = Services_list.GetAt(5);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult charac_r_obj = main_service.GetCharacteristicsAsync().get();
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> Charac_list = charac_r_obj.Characteristics();
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic INFO_CHAR = Charac_list.GetAt(0);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_CHAR = Charac_list.GetAt(1);
	//TODO: ->(2) "Make the write command a seperate function that uses the maps from (2)" (3).
	winrt::Windows::Storage::Streams::IBuffer send_packet = winrt::Windows::Security::Cryptography::CryptographicBuffer::DecodeFromHexString(L"0100");
	COMMAND_CHAR.WriteValueAsync(send_packet);

	for (int i = 0; i < 100; i++)
	{
		auto read_result = INFO_CHAR.ReadValueAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
		auto read = read_result.Value();
		auto result = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(read);
		std::string readable_result = winrt::to_string(result);
		std::cout << "Data: " << readable_result << std::endl;
	}
	std::cin.get();
}
int main()
{
	uint64_t address = 0x2CBABA2FE1DC;
	GearVRcontroller controller(address);
	controller.Start_receive();
}