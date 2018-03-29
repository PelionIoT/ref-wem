
## Workplace Environmental Monitor Reference Deployment

<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/IMG_1254.png)<span>Photo</span></span>

This document contains the information you need to configure your Workplace Environmental Monitor and set it up, so you can demonstrate its functionality to a group.

The monitor has sensors for:

- Temperature.
- Humidity.
- Light brightness.

To turn the device on and off, locate the power switch on the side of the device next to the micro-USB port. When fully charged, the device can run for several hours on internal battery. You can also connect it to a power source through the micro-USB port for power and charging.

When turned on, the device connects to the Wi-Fi access point it was configured to. Because of this, the device probably will not connect to the internet the first time you turn it on. Follow the instructions below to flash the board. You need your Arm Mbed credentials, Wi-Fi access point and password.

The documentation on this page shows how to flash software onto the device and also how to wirelessly update the firmware (also known as "firmware over the air" updates).

You need to flash the firmware at least once because it's the only way to get your device associated with your Arm Mbed Cloud account. We have a web page set up that compiles the image for you, so you only need to drag and drop the file onto your device.

#### Required hardware:

- Laptop with USB port (or dongles to provide a USB port).
- USB to micro-USB cable.
- The environmental monitor device.

#### Required file:

- An Mbed Cloud certificate file. Visit https://portal.us-east-1.mbedcloud.com/signup to sign up if you haven't already.
To get an Mbed Cloud certificate file, sign in at https://portal.us-east-1.mbedcloud.com/

Click "Device identity", and then click on "Actions". Then click on "Create a developer certificate":

<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/mbed_create_cert.png)<span>Photo</span></span>

Click on the certificate, and then click the button `Download the developer C file`:

<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/mbed_cloud_cert_download.png)<span>Photo</span></span>

#### Now that you have an Arm Mbed developer C file, follow the these steps to flash your device:

1. Connect your laptop to the device with a USB cable. The laptop automatically adds a new drive, in this case named `DAPLINK`. This is where you drag and drop the firmware:


<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/image3.jpeg)<span>Photo</span></span>

1. Visit and clone the firmware repository: https://github.com/ARMmbed/ref-wem-firmware

1. Follow the instructions to build your firmware: https://github.com/ARMmbed/ref-wem-firmware/blob/master/README.md 

1. Save that file to your computer, and then drag and drop it into the drive for your device (see step #1).

That's it! Wait about a minute, and the device is now flashed with your image.

### Using the command console on the device

<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/ScreenShot2017-12-05at2.34.23PM.jpeg)<span>Photo</span></span>

<span class="images">![](https://s3.us-west-2.amazonaws.com/reference-docs-images/work_environ_mon_docs/IMG_2039.jpg)<span>Photo</span></span>

Connect your device to your laptop with a USB cable. Then you can use software such as PuTTy, Coolterm or Minicom to connect to the device.

On Mac OS, for example, you can type a command such as:

```
    minicom -D /dev/tty.usbmodem144142
```

To set the Wi-Fi configuration:

Press the `ENTER` key, and you see a prompt where you can type `help` to see all the available commands.

```
    set wifi.ssid NAME
    set wifi.key PASSWORD
    set wifi.encryption TYPE
```

Replace `NAME` with the displayed name of your access point and similar with `PASSWORD`. The `TYPE` should be wither WPA2 or OPEN are the two most popular modes.

Type `reboot` to let the device boot up and connect to the Wi-Fi access point you specified.

### FOTA update

Before starting the FOTA campaign, you must increment the version of the application because you can't update to the same revision of the firmware.

In your cloned project folder, open the file `mbed_app.json` with the editor of your choice and increment the version number.

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

For a deeper understanding of FOTA, please see:
https://cloud.mbed.com/docs/v1.2/updating-firmware/index.html 

For more details on building, please see:
https://github.com/ARMmbed/ref-wem-firmware/blob/master/README.md 

<span class="notes">**Note:** To ensure that FOTA works after running `make distclean`, you must first run `make certsave`, so your certs pass to the new clean environment.</span>

