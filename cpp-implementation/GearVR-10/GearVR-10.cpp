// GearVR-10.cpp : This file contains the 'main' function. Program execution begins and ends there.
//just wanted to say i hate MS' stupid projection, STOP USING ARBITRARY TYPES GODDAMN IT.
//TODO: "Switch to async and multithreaded worflow." (0)
//TODO: "Use device enumration instead of a direct MAC, this will solve a lot of the inconvinent stuff here." (1)

#include "WrapperHeader.h"

//TODO: "Create a modes enum" (2).

struct xyAxisTemp
{
	int Xaxis;
	int	Yaxis;
};

class Current_Res
{
public:
	int width;
	int height;
	Current_Res();
	~Current_Res();
};

Current_Res::Current_Res()
{
	RECT desktop;
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);
	width = desktop.right;
	height = desktop.bottom;
}

Current_Res::~Current_Res()
{
}

struct misc_data
{
	float timestamp;
	int temperature;
};

class GearVRcontroller
{
public:
	GearVRcontroller(uint64_t addr);
	~GearVRcontroller();

public:
	enum class data_mode
	{
		raw = 0, formatted = 1
	};
	std::vector<int> ReturnReading(data_mode mode);
	std::string raw_result;
private:
	uint64_t MAC_Address;
	winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService> Services_List;
};

GearVRcontroller::GearVRcontroller(uint64_t address)
{
	////Initializing address and device.
	GearVRcontroller::MAC_Address = address;
	winrt::Windows::Devices::Bluetooth::BluetoothLEDevice device = winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(MAC_Address).get();

	//Initializing the controller's main service and its main chars (not async).
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceServicesResult Service_r_obj = device.GetGattServicesAsync().get();
	GearVRcontroller::Services_List = Service_r_obj.Services();
}

GearVRcontroller::~GearVRcontroller()
{
	//device.Close();
	//std::cout << "Connection with  " << winrt::to_string(device.Name()) << " closed" << std::endl;
}

std::vector<int> GearVRcontroller::ReturnReading(data_mode mode) //TODO: return as a two type tuple instead.
{
	//Initializing the main service and its characteristics
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattDeviceService main_service = Services_List.GetAt(5);
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
	if (int x = int(mode); x == 1)
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
		raw_result = raw_readable_result;
		std::vector<int> result_vec;
		return result_vec;
	}
}

class ControllerData
{
public:
	ControllerData(GearVRcontroller& con_obj);
	~ControllerData();
	xyAxisTemp return_scaled_touchpad();
	misc_data return_misc();
	void set_buttons();
	void update();

	bool triggerButton = 0;
	bool homeButton = 0;
	bool backButton = 0;
	bool touchpadButton = 0;
	bool volumeUpButton = 0;
	bool volumeDownButton = 0;

private:
	GearVRcontroller * curr_con;
	std::vector<int> buffer_array;
	xyAxisTemp current_touch;
	std::vector<int> accel_readings;
	std::vector<int> gyro_readings;
};

ControllerData::ControllerData(GearVRcontroller& con_obj)
{
	GearVRcontroller * con_ptr = &con_obj;
	ControllerData::curr_con = con_ptr;
}

ControllerData::~ControllerData()
{
}

void ControllerData::update()
{
	std::vector<int> buff_vec = curr_con->ReturnReading(GearVRcontroller::data_mode::formatted);
	ControllerData::buffer_array = buff_vec;
}

misc_data ControllerData::return_misc()
{
	ControllerData::update();
	float temp_timestamp = ((buffer_array[0] + buffer_array[1] + buffer_array[2]) & 0xFFFFFFFF) / static_cast<float>(1000) * 0.001;
	int temp_temperature = buffer_array[57];
	misc_data time;
	time.temperature = temp_temperature;
	time.timestamp = temp_timestamp;
	return time;
}

xyAxisTemp ControllerData::return_scaled_touchpad()
{
	ControllerData::update();
	Current_Res RES1366;
	int tempx = (((buffer_array[54] & 0xF) << 6) + ((buffer_array[55] & 0xFC) >> 2)) & 0x3FF;
	int tempy = (((buffer_array[55] & 0x3) << 8) + ((buffer_array[56] & 0xFF) >> 0)) & 0x3FF;
	current_touch.Xaxis = ((RES1366.width / 315) * tempx);
	current_touch.Yaxis = ((RES1366.height / 315) * tempy);
	return current_touch;
}

void ControllerData::set_buttons()
{
	ControllerData::update();
	triggerButton = buffer_array[58] & (1 << 0);
	homeButton = buffer_array[58] & (1 << 1);
	backButton = buffer_array[58] & (1 << 2);
	touchpadButton = buffer_array[58] & (1 << 3);
	volumeUpButton = buffer_array[58] & (1 << 4);
	volumeDownButton = buffer_array[58] & (1 << 5);
}
