# GearVR-controller-win10

A Windows interface implementation for the Samsung GearVR Controller.

> ⚠️ **Breaking Change: Configuration Update Required**
>
> In this release, hardcoded calibration parameters have been moved to the configuration file (`config.ini`).
> Users must manually append the parameters from the code into two new sections in their `.ini` file to ensure proper functionality as follows.
>
> ### Example Configuration (`config.ini`)
>
> ```
> [engine-params]
> sensor-gain=0.585
> magnet-enable=0
> reject-enable=0
> reject-accel=0.01
> reject-magnet=0.15
>
> [sensor-params]
> gyro-sens-x=0.85
> gyro-sens-y=0.95
> gyro-sens-z=0.95
> gyro-offset-x=0.7548
> gyro-offset-y=0.7402
> gyro-offset-z=1.1433
> accel-sens-x=0.1
> accel-sens-y=0.85
> accel-sens-z=0.85
> accel-offset-x=0
> accel-offset-y=0
> accel-offset-z=0
> magnet-offset-x=0
> magnet-offset-y=0
> magnet-offset-z=0
> ```
>
> ### Migration Steps:
>
> 1. Copy the above keys into `config.ini` in your project directory.
> 2. Ensure all required fields match the previous hardcoded values in your project.
> 3. Restart the application to apply the changes.

## Getting Started

1. Pairing the controller:

    - Long-press the home button until it flashes red, green, and blue
    - Navigate to Windows 10 settings and pair the device - when successful, it will display the battery level
    - Use a Bluetooth LE explorer application like [Bluetooth LE Lab](https://apps.microsoft.com/detail/9n6jd37gwzc8) to obtain the device's MAC address

2. Project setup:

    - Clone the repository
    - Open the `.sln` file in Visual Studio
    - Build the project to resolve WinRT dependencies

3. Initial configuration:

    - Run the built `.exe`
    - Enter the MAC address when prompted
    - The program will generate a `config.ini` with default keybindings
    - Button mappings can be customized using virtual key codes found in the [Windows documentation](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)
    - Restart the program to apply settings

4. Operation:
    - Select the desired operating modes for the controller
    - Input `5` to enable controller functionality

## Features

-   Command-line interface
-   Customizable button mapping
-   Touchpad tracking functionality
-   Dual-mode touchpad operation:
    -   Mode 1: Single-button touchpad
    -   Mode 2: Five-button configuration (center + four directional)
-   Experimental 3D cursor support:
    -   Currently requires calibration to address drift and jitter
    -   Not compatible with simultaneous touchpad tracking

## Credits

-   [@jsyang](https://github.com/jsyang) for comprehensive reverse engineering documentation
-   [@xioTechnologies](https://github.com/xioTechnologies) for the Madgwick fusion algorithm implementation
-   [@metayeti](https://github.com/metayeti) for the INI file parsing library

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

### Third Party Licenses

This project incorporates third-party libraries distributed under their own terms. See [NOTICE](NOTICE) for detailed licensing information.
