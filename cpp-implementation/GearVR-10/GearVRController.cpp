#include "WrapperHeader.h"
#include "GearVRController.h"

GearVRController::GearVRController(uint64_t address)
	: device(winrt::Windows::Devices::Bluetooth::BluetoothLEDevice::FromBluetoothAddressAsync(address).get()),
	ser_res_obj(device.GetGattServicesAsync().get()), services_list(ser_res_obj.Services()), main_service(services_list.GetAt(5)),
	charac_res_obj(main_service.GetCharacteristicsAsync().get()),
	charac_list(charac_res_obj.Characteristics()), INFO_CHAR(charac_list.GetAt(0)), COMMAND_CHAR(charac_list.GetAt(1))
	
{
	GearVRController::MAC_address = address;
}

GearVRController::~GearVRController()
{
}

std::vector<int> GearVRController::returnReadingVector()
{
	// Read operation through a windows ibuffer
	winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattReadResult read_result = INFO_CHAR.ReadValueAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
	winrt::Windows::Storage::Streams::IBuffer read = read_result.Value();
	winrt::hstring result = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(read);
	std::string raw_readable_result = winrt::to_string(result);

	// Decoding to from the hex string to a vector
	std::vector<int> result_vec;
	for (int i = 0; i < 120; i += 2)
	{
		std::string temp_str;
		temp_str.append(1, raw_readable_result[i]);
		temp_str.append(1, raw_readable_result[i + 1]);
		std::stringstream temp_str_stream;
		temp_str_stream << std::hex << temp_str;
		unsigned int x;
		temp_str_stream >> x;
		result_vec.push_back(x);
	}
	return result_vec;
}

std::string GearVRController::returnRawReading()
{
	auto read_result = INFO_CHAR.ReadValueAsync(winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();
	auto read = read_result.Value();
	auto result = winrt::Windows::Security::Cryptography::CryptographicBuffer::EncodeToHexString(read);
	std::string raw_readable_result = winrt::to_string(result);
	return raw_readable_result;
}

void GearVRController::writeCommand()
{
	winrt::Windows::Storage::Streams::IBuffer send_packet = winrt::Windows::Security::Cryptography::CryptographicBuffer::DecodeFromHexString(L"0100");
	COMMAND_CHAR.WriteValueAsync(send_packet);
}

// Return current screen resolution
CurrentResolution::CurrentResolution()
{
	RECT desktop;
	const HWND h_desktop = GetDesktopWindow();
	GetWindowRect(h_desktop, &desktop);
	width = desktop.right;
	height = desktop.bottom;
}

CurrentResolution::~CurrentResolution()
{
}

ControllerData::ControllerData(GearVRController& con_obj)
{
	GearVRController* temp_ptr = &con_obj;
	ControllerData::current_controller = temp_ptr;
}

ControllerData::~ControllerData()
{
	delete current_controller;
}

void ControllerData::fullRefresh()
{
	std::vector<int> temp_buffer_vec = current_controller->returnReadingVector();
	ControllerData::buffer_vector = temp_buffer_vec;
}

MiscExtraData ControllerData::returnMiscExtra()
{
	ControllerData::fullRefresh();
	float temp_timestamp = ((buffer_vector[0] + buffer_vector[1] + buffer_vector[2]) & 0xFFFFFFFF) / static_cast<float>(1000) * 0.001;
	int temp_temperature = buffer_vector[57];
	MiscExtraData time;
	time.temperature = temp_temperature;
	time.timestamp = temp_timestamp;
	return time;
}

AxisData ControllerData::returnScaledTouchpadOutput()
{
	ControllerData::fullRefresh();
	CurrentResolution RES1366;
	AxisData current_touch;
	int tempx = (((buffer_vector[54] & 0xF) << 6) + ((buffer_vector[55] & 0xFC) >> 2));
	int tempy = (((buffer_vector[55] & 0x3) << 8) + ((buffer_vector[56] & 0xFF) >> 0));
	current_touch.axis_x = ((RES1366.width / 315) * tempx);
	current_touch.axis_y = ((RES1366.height / 315) * tempy);
	return current_touch;
}

std::bitset<6> ControllerData::returnButtons()
{
	ControllerData::fullRefresh();
	std::bitset<6> button_set;
	button_set[0] = (bool)(buffer_vector[58] & (1 << 0)); //0 -> trigger button
	button_set[1] = (bool)(buffer_vector[58] & (1 << 1)); //1 -> home button
	button_set[2] = (bool)(buffer_vector[58] & (1 << 2)); //2 -> back button
	button_set[3] = (bool)(buffer_vector[58] & (1 << 3)); //3 -> touchpad button
	button_set[4] = (bool)(buffer_vector[58] & (1 << 4)); //4 -> vol up button
	button_set[5] = (bool)(buffer_vector[58] & (1 << 5)); //5 -> vol down button
	return button_set;
}