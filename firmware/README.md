## Workplace Environmental Monitor

### Purpose

This is an Arm Mbed reference deployment application of a workplace environmental monitor. The monitor contains environmental sensors for light, temperature and humidity. The sensor values are uploaded to Arm Mbed Cloud 1.2. The application also demonstrates firmware updates using the firmware-over-the-air (FOTA) capabilities of Mbed and Mbed Cloud. A desktop plastic case contains the monitor. It is battery powered and has LED indicators and an LCD display.

<span class="images">![](https://s3-us-west-2.amazonaws.com/mbed-os-docs-images/photo.png)<span>Photo</span></span>

### Prerequisites

To build this project, you need to install the following:

1. [GNU Arm Compiler](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads) version 6.3.1.20170215 or greater

    Here is an example showing how to install on a Mac:

    ```
    brew tap ARMmbed/homebrew-formulae
    brew install arm-none-eabi-gcc
    ```

2. Python 2.7

3. [Python virtualenv](https://virtualenv.pypa.io/en/stable/userguide/)

    It is *strongly* recommended to use a Python virtualenv to isolate your build environment from the underlying system.  If you do this step before you clone the ref-wem-firmware repo, then the venv folder will be placed outside of the ref-wem-firmware folder which is recommended.  If instead you place venv in the root of the ref-wem-firmware project, you will need to add venv to .mbedignore to prevent mbed-cli from attempting to build the files inside it.

    ```
    virtualenv --no-site-packages --python=$(which python) venv
    source venv/bin/activate
    ```

4. Copy your SSH public key to github

    The build preparation step involves cloning libraries from github using ssh, so you need to add an ssh key from your development system to your github account. Please see instructions [here](https://help.github.com/articles/connecting-to-github-with-ssh/).

### Importing `ref-wem-firmware`

Arm Mbed CLI can import the project, along with the Arm Mbed OS codebase and all dependent device drivers.

To import the project from the command-line:

1. Navigate to a workspace directory of your choice.

	``cd ~/workspace``

2. Import the example:

	```
	git clone git@github.com:ARMmbed/ref-wem-firmware.git
	cd ref-wem-firmware
	```

    `ref-wem-firmware` is now under `~/workspace/ref-wem-firmware`. You can look at `main.cpp` to familiarize yourself with the code.

3. Install the Python dependencies:

	```
	pip install -r requirements.txt
	```

### Specifying a network configuration

The project configuration file `mbed_app.json` specifies the network configuration. Open `mbed_app.json`, and modify the following configuration values to suit the deployment environment:

```
    ...
    "wifi-ssid": {
        "help": "The SSID to connect to if using a WiFi interface",
        "value": "\"MYSSID\""
    },
    "wifi-security": {
        "help": "WPA, WPA2, WPA/WPA2, WEP, NONE, OPEN",
        "value": "\"WPA2\""
    },
    "wifi-password": {
        "help": "An optional password for wifi security authentication",
        "value": "\"MYPASSWORD\""
    }
    ...
```

You can also change the configuration at runtime by using a serial console. See the section ["Serial command help"](#serial-command-help)  for help with connecting to and using the console. See the section on ["Wi-Fi commissioning"](#wi-fi-commissioning) for the relevant keystore options.

### <a name="GetDevCert"></a>Downloading a developer certificate

A certificate is required for the end device to be able to communicate with Mbed Cloud. Log on to the Mbed Cloud portal, and navigate to `Device Identity -> Certificates`.

If creating a new certificate, select the `Actions` pulldown, and choose `Create a developer certificate`. Fill in the form, and click `Create Certificate`. At this time, you may download the certificate onto the local system. Place the certificate C file in the root folder of the project.

If downloading an existing certificate, choose the name of the appropriate certificate from the list of certificates presented on the Certificates page. Click `Download Developer C file`, and place the certificate C file in the root folder of the project.

### Compiling

The project uses a Makefile to compile the source code. The Makefile detects the toolchain and target and calls the Mbed compiler with appropriate options. To build for the current hardware, you may need to set your Mbed target to UBLOX_EVK_ODIN_W2.

Assuming you are compiling with GCC, your `.mbed` file should look like the following:

```
$ cat .mbed
ROOT=.
TARGET=UBLOX_EVK_ODIN_W2
TOOLCHAIN=GCC_ARM
```

If the file does not exist, you can either allow the Makefile to create it with default settings or create it yourself with the following commands.

```
$ mbed config ROOT .
$ mbed target UBLOX_EVK_ODIN_W2
$ mbed toolchain GCC_ARM
```

Typing `make` builds the bootloader and application and combines them into a single image. The final images are copied into the `bin/` folder.

```
$ make
```

#### Compilation errors

The project fails to compile if a developer certificate is not present in the local source directory. This file is typically named `mbed_cloud_dev_credentials.c` and defines several key constants. Please see [Downloading a developer certificate](#GetDevCert) for more information.

A missing certificate results in compilation errors similar to the following:

```
./BUILD/UBLOX_EVK_ODIN_W2/GCC_ARM/mbed-cloud-client-restricted/factory_client/factory_configurator_client/source/fcc_dev_flow.o: In function `fcc_developer_flow':
fcc_dev_flow.c:(.text.fcc_developer_flow+0x130): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_ENDPOINT_NAME'
fcc_dev_flow.c:(.text.fcc_developer_flow+0x138): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE'
fcc_dev_flow.c:(.text.fcc_developer_flow+0x13c): undefined reference to `MBED_CLOUD_DEV_BOOTSTRAP_SERVER_ROOT_CA_CERTIFICATE_SIZE'
...
```

#### Cleaning the build

```
make clean
```

`make clean` cleans the C++ compile output.

```
make distclean
```

`make distclean` removes all dependency files and generated files.

### Flashing your board

The following command copies `bin/combined.bin` to a USB-attached device.

```
make install
```

If this command fails or you have more than one device attached to your build system, you can manually copy the image to the device. For example:

```
$ cp bin/combined.bin /Volumes/DAPLINK/
```

Be sure to substitute for the correct mount point of your device.

### Update over the air

Login to the mbed cloud portal and copy your mbed cloud API key, which is a one-line string, into a new file called `.mbed-cloud-key`. Store this file in the root directory of this project. For more information, see [Access Mbed Cloud with API Keys](https://cloud.mbed.com/docs/v1.2/connecting/api-keys.html).

NOTE: If you are updating a device that was previously provisioned to a different Mbed cloud account, you must `reset certs` on the local device before initiating a campaign, otherwise the FOTA operation will encounter authorization failures.  See the section ["Serial command help"](#serial-command-help) for details on how to run commands on the device.

Before starting the FOTA campaign, you should increment the version of your application.

Open the file `mbed_app.json` in the editor of your choice, and increment the version number.

Change from:

```
"version": {
    "help": "Display this version string on the device",
    "value": "\"1.0\""
},
```

to:

```
"version": {
    "help": "Display this version string on the device",
    "value": "\"1.1\""
},
```

Next, open your shell to your project folder. 

```
make campaign
```

This launches the FOTA campaign and begins updating your devices.

### Serial command help

You can connect a serial terminal to the device for the purpose of viewing diagnostic output and issuing serial commands. Serial connection is at a baud rate of 115200.

Press enter at any time to get a command prompt.

```
>
```

Typing `help` at the prompt provides a list of the commands and a brief set of usage instructions.

```
> help
Help:
del          - Delete a configuration option from the store. Usage: del <option>
get          - Get the value for the given configuration option. Usage: get [option] defaults to *=all
help         - Get help about the available commands.
reboot       - Reboot the device. Usage: reboot
reset        - Reset configuration options and/or certificates. Usage: reset [options|certs|all] defaults to options
set          - Set a configuration option to a the given value. Usage: set <option> <value>
```

#### Option keystore

The keystore is a name value pair database that stores configuration parameters, for example Wi-Fi credentials.

The following commands are provided to manipulate the keystore:

- `get` get a key and print its value.
  
  ```
  > get wifi.ssid
  
  wifi.ssid=iotlab
  ```

- `set` set a key to the given value.
  
  ```
  > set wifi.ssid iotlab
  
  wifi.ssid=iotlab
  ```

- `del` delete a key and it's value.
  
  ```
  > del wifi.ssid
  
  Deleted key wifi.ssid
  ```

#### Wi-Fi commissioning

To configure Wi-Fi, set the following key options:

```
> set wifi.ssid yourssid
wifi.ssid=yourssid

> set wifi.key passphrase
wifi.key=passphrase

> set wifi.encryption WPA2
wifi.encryption=WPA2
```

After setting the Wi-Fi credentials, reset the device:

```
> reboot
```

#### Reset

To delete all stored options and their values:

```
> reset           deletes the options keystore

> reset options   deletes the options keystore

> reset certs     deletes the fcc certs

> reset all       deletes fcc certs and options keystore
```

### M2M resources

This project's firmware exposes several M2M resource IDs. Most of these resources are read-only sensor measurements.

The firmware also exposes the following resources:

#### Application information

* Object ID: 26241

##### Application label

* Resource ID: 1
* Path: /26241/0/1

The application label is a user-friendly name that can be read and written. The application label is displayed on the LCD with a prefix of `Label: `.

You can write the application label in 3 ways:

- By setting `app-label` in the config section of `mbed_app.json`:

    ```
        "config": {
            "app-label": {
                "help": "Sets a device friendly name displayed on the LCD",
                "value": "\"dragonfly\""
            },
        ...
    ```

- Through M2M PUT requests
  
  This can be demonstrated on the Mbed Cloud portal. After you register a device with Mbed Cloud, the `Connected Devices` page lists the device. Click on the Device ID to bring up device details, and then click on the `Resources` tab. Scroll to `Object ID 26241`, and click `/26241/0/1`. On the popup dialog, click `Edit`. Enter a new name in the `Value` text box. Make sure that the `PUT` request type is chosen, and then click the `Send` button.

- Through the serial console by setting the `app.label` key:

    ```
    > set app.label anisoptera
    app.label=anisoptera
    ```

    <span class="notes">**Note:** When changing the application label by using the serial console, you must perform a reboot for the setting to take effect.</span>

##### Application version

* Resource ID: 2
* Path: /26241/0/2

The application version is read-only. Its value populates from the `version` field in `mbed_app.json`.

#### User-configured geographical information

* Object ID: 3336
* Instance: 0

You can store latitude, longitude and accuracy information on the device and make them available using M2M in instance 0 of object ID 3336. Along with latitude and longitude, a resource of named `Application Type` is also present and is set to `user` in the case of instance 0. This allows other types of geographical data to be made available on a separate instance, which a type can differentiate.

You can write geographical data in 3 ways:

- By setting the following keys in the `config` section of `mbed_app.json`:

    ```
        "config": {
            "geo-lat": {
                "help": "Sets the device latitude, from -90.0 to 90",
                "value": "\"30.2433\""
            },
            "geo-long": {
                "help": "Sets the device longitude, from -180 to 180",
                "value": "\"-97.8456\""
            },
            "geo-accuracy": {
                "help": "Sets the accuracy of geo-lat and geo-lon, in meters",
                "value": "\"11\""
            },
        ...
    ```

- Through the serial console by setting the following keys:

    ```
    > set geo.lat 30.2433
    geo.lat=30.2433

    > set geo.long -97.8456
    geo.long=-97.8456

    > set geo.accuracy 11
    geo.accuracy=11
    ```

- Through M2M PUT requests
  
  This can be demonstrated on the Mbed Cloud portal. After you register a device with Mbed Cloud, the `Connected Devices` page lists the device. Click on the Device ID to bring up device details, and then click on the `Resources` tab. Scroll to `Object ID 3336`, and click on resources attached to instance 0, such as `/3336/0/5514`, which shows latitude; `/3336/0/5515` which shows longitude; and `/3336/0/5516`, which shows accuracy. On the popup dialog, click `Edit`, and enter a new value in the `Value` text box. Make sure that the `PUT` request type is chosen. Then click the `Send` button.

Here are some additional details about each of the user-configurable geographical resources. Unless otherwise specified, each resource has a keystore option that you can modify on the serial console. Please see the section entitled [Option keystore](#option-keystore) for more details about how to use this interface.

You can delete geographical data from the Mbed Cloud portal by setting the resource to a `-` (dash). This special character allows upstream web applications to clean up state and make any appropriate changes.

    ```
    > set geo.lat -
    geo.lat=-

    > set geo.long -
    geo.long=-

    > set geo.accuracy -
    geo.accuracy=-

    > reboot
    ```

##### Type

* Resource ID: 5750
* Path: /3336/0/5750
* option key: none - This resource is not configurable
* value: "user"

##### Latitude

* Resource ID: 5514
* Path: /3336/0/5514
* option key: geo.lat

##### Longitude

* Resource ID: 5515
* Path: /3336/0/5515
* option key: geo.long

##### Accuracy

* Resource ID: 5516
* Path: /3336/0/5516
* option key: geo.accuracy
