#pragma once

// Standard C/C++ headers.
#include <algorithm>
#include <bitset>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <vector>

// Windows headers.
#include "wtypes.h"
#include <WinUser.h>
#include <windows.h>

// C++/Winrt headers.
#include <winrt/Windows.Devices.Bluetooth.GenericAttributeProfile.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Security.Cryptography.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Storage.h>

// IMU Fusion courtesy of xioTechnologies
#include "./Fusion/Fusion.h"