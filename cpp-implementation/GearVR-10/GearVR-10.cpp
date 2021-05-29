// GearVR-10.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "WrapperHeader.h"

enum modes
{
	disabled = 0x0000,
	normal = 0x0100,
	vr = 0x0800
};


int main()
{
	//Initializing address and service UUID.
	winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached;
	uint64_t MAC_addr = 0x2CBABA2FE1DC;
	winrt::guid const IOGUUID = winrt::guid("4F63756C-7573-2054-6872-65656D6F7465");

	//Initializing the controller and printing the mac address.
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(MAC_addr).get();
	std::cout << winrt::to_string(device.Name()) << " at " << std::to_string(MAC_addr) << std::endl;

	//Initializing the controller's main service and its main chars (not async).
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult service_r_obj = device.GetGattServicesForUuidAsync(IOGUUID).get();
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> ser_list = service_r_obj.Services();
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService main_service = ser_list.GetAt(0);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult charac_r_obj = main_service.GetCharacteristicsAsync().get();
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> charac_list = charac_r_obj.Characteristics();

	
	
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic INFO_CHAR = charac_list.GetAt(0);
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_CHAR = charac_list.GetAt(1);

	//Creating a buffer type to send to 81char and writting it.
	winrt::Windows::Storage::Streams::IBuffer send_packet = winrt::Windows::Security::Cryptography::CryptographicBuffer::DecodeFromHexString(L"0100");
	COMMAND_CHAR.WriteValueAsync(send_packet);

	//Reading the buffer.
	
	while (true)
	{
		auto read_result = INFO_CHAR.ReadValueAsync().get();
		auto read = read_result.Value();
		auto result = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(read);
		std::string readable_result = winrt::to_string(result);
		std::cout << readable_result << std::endl;
	}
	

	
}