#pragma once

#include "WrapperHeader.h"

class GearVRController
{
public:
	GearVRController(uint64_t addr);
	~GearVRController();

public:
	std::vector<int> returnReadingVector();
	std::string returnRawReading();
	void writeCommand();

private:
	
	uint64_t MAC_address;
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult ser_res_obj;
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> services_list;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService main_service;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristicsResult charac_res_obj;
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic> charac_list;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic INFO_CHAR;
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCharacteristic COMMAND_CHAR;


};

struct AxisData
{
	int axis_x;
	int	axis_y;
};

class CurrentResolution
{
public:
	int width;
	int height;
	CurrentResolution();
	~CurrentResolution();
};


struct MiscExtraData
{
	float timestamp;
	int temperature;
};

class ControllerData
{
public:
	ControllerData(GearVRController& con_obj);
	~ControllerData();
	AxisData returnScaledTouchpadOutput();
	MiscExtraData returnMiscExtra();
	std::bitset<6> returnButtons();
	void fullRefresh();
private:
	GearVRController* current_controller;
	std::vector<int> buffer_vector;
	
};