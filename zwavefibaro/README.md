# Z-Wave Fibaro

This plugin adds support for Z-Wave devices by Fibaro.

## Supported Things

* Fibaro Wall Plug
* Fibaro Motion Sensor

## Pairing

In order to connect a Fibaro device to a Z-Wave network
* In nymea, open System Settings -> Z-Wave -> select the network and press on "Add new device"
* Press the button on the Fibaro device quickly 3 times

To remove a Fibaro device
* Select "Remove a device" in the nymea Z-Wave settings
* Press the button on hte Fibaro device quickly 3 times

If a Fibaro device is paired with a network but can't be removed properly from that any more,
it needs to be factory reset in order to connect it to a new network.

To factory reset a Fibaro device
* Press and hold the button on the Fibaro device
* Keep pressed until the device light color changes to yellow
* Release the button
* While the device light is still yellow, shorty press the button once
* The device will confirm the reset by lighting up red.

After a Factory reset, the device can be added to a Z-Wave network with the above steps.
