# nymea plug-ins for Z-Wave devices

--------------------------------------------
<p align="center">
  <a  href="https://nymea.io">
    <img src="https://nymea.io/downloads/img/nymea-logo.svg" width=300>
  </a>
</p>


This repository contains integrations for nymea. 

nymea (/[n'aiːmea:]/ - is an open source IoT edge server. The plug-in based architecture allows to integrate protocols and APIs. With the build-in rule engine you are able to interconnect devices or services available in the system and create individual scenes and behaviours for your environment.


## Getting help

If you want to present your project or want to share your newest developments you can share it in
[Our Forum](https://forum.nymea.io)

If you are facing any troubles, don't hesitate to reach out for us or the community members, we will be pleased to help you:
Chat with us on [Telegram](http://t.me/nymeacommunity)

## Documentation

* A detailed description how to install and getting started with the *nymea* can be found here:

    [nymea | user documentation](https://nymea.io/documentation/users/installation/getting-started).

* A detailed documentation for developers can be found here:

    [nymea | developer documentation](https://nymea.io/documentation/developers/).


## Contributing

To contribute support for a Z-Wave device, you're welcome to file a pull request to this repository. If you need help in creating an integration plugin, feel free to ask in our forum or the Telegram group as well.

If you don't have any developer skills at all, still feel free to bring the topic up. Our community is happy to help wherever possible.

## How to add support for a new device

When a Z-Wave device is not yet supported in nymea, it can still be added to the Z-Wave network, but no thing will appear. However, nymea will
inspect the device and print detailed information about the device in the logs which normally should be enough to be able to to add support for a device.

Select the plugin for the appropriate manufacturer (or create a new one if there is none for this manufacturer), add a thing class for the device
in the integrationpluginxyz.json file and fill in the handleNode(), setupThing() and, if needed, executeAction() methods in the .cpp file. The existing
code for other devices should work as example code as all Z-Wave devices work in the same way and handling their features is normally just a few lines of code.

Please refer to the [nymea | developer documentation](https://nymea.io/documentation/developers/) on how to get started with plugin development if the 
above is new to you.

## License
--------------------------------------------
> nymea is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3 of the License.
