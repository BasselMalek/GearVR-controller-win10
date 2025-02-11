# GearVR-controller-win10

A Windows interface implementation for the Samsung GearVR Controller.

> ⚠️ **Breaking Change: Configuration Update Required**
>
> In this release, hardcoded calibration parameters have been moved to the configuration file (`config.ini`) and the app runs through a GUI now.
> Users must manually append the parameters from the code into two new sections in their `.ini` file to ensure proper functionality as follows.
>
> ## Migration Steps
>
> 1. Save the keybindings in the old `.ini` and any calibration numbers somewhere safe and delete the file.
> 2. Generate the `.ini` file again by running the program and inputing your MAC.
> 3. Fill in the keybindings, sensitivities, and calibration data in the new `.ini` file from the previous hardcoded values in your project or further calibrate your controller if needed.
> 4. Restart the application to apply the changes.

## Getting Started

1. Pairing the controller:

    - Long-press the home button until it flashes red, green, and blue
    - Navigate to Windows 10 settings and pair the device - when successful, it will display the battery level
    - Use a Bluetooth LE explorer application like [Bluetooth LE Lab](https://apps.microsoft.com/detail/9n6jd37gwzc8) to obtain the device's MAC address

2. Project setup:

    - Clone the repository
    - Download and setup wxWidgets through their [tutorial](https://docs.wxwidgets.org/3.2/plat_msw_binaries.html)
    - Open the `.sln` file in Visual Studio
    - Build the project to resolve WinRT dependencies
    - Extract the release DLLs for wxWidgets into the root directory for your exe

3. Initial configuration:

    - Run the built `.exe`
    - Press start from the tray icon context menu
    - The program will generate a `config.ini` with default keybindings and a placeholder MAC address
    - Put in the MAC address you extracted earlier from BLE labs in decimal in its field
    - Button mappings can be customized using virtual key codes found in the [Windows documentation](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)

4. Operation:
    - Select the desired operating modes for the controller from the config menu
    - Press Start to enable controller functionality

## Features

-   Customizable button mapping
-   Touchpad tracking functionality
-   Dual-mode touchpad operation:
    -   Mode 1: Single-button touchpad
    -   Mode 2: Five-button configuration (center + four directional)
-   Motion cursor support:
-   Allows calibration to address drift and jitter
-   Not compatible with simultaneous touchpad tracking

## Credits

-   [@jsyang](https://github.com/jsyang) for comprehensive reverse engineering documentation
-   [@xioTechnologies](https://github.com/xioTechnologies) for the Madgwick fusion algorithm implementation
-   [@metayeti](https://github.com/metayeti) for the INI file parsing library

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third Party Licenses

This project incorporates third-party libraries distributed under their own terms. See [NOTICE](NOTICE) for detailed licensing information.
