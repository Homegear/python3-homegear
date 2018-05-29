libhomegear-python
==================

libhomegear-python is a python extension to connect to Homegear over Unix Domain Sockets. It supports all of Homegear's RPC methods and live event reception.

## Prerequisites

The extension requires `libhomegear-ipc` to be installed. To install it, add the Homegear APT repository for your distribution (see https://homegear.eu/downloads.html) and execute

```
apt install libhomegear-ipc
```

## Setup

To compile and install the extension manually, execute

```
sudo python3 setup.py install
```

## Methods

There is only one object available: `Homegear`. It takes two parameters in it's constructor: The path to the Homegear IPC socket (`/var/run/homegear/homegearIPC.sock` by default) and a callback method. The callback method is executed when a device variable is updated in Homegear. On instantiation the class waits until it is connected succesfully to Homegear. After 2 seconds it returns even if there is no connection. To check, if the object is still connected, you can call `connected()`. Apart from this method, you can call all RPC methods available in Homegear.

## Usage example

A minimal example:

```
from homegear import Homegear

# This callback method is called on Homegear variable changes
def eventHandler(peerId, channel, variableName, value):
	# Note that the event handler is called by a different thread than the main thread. I. e. thread synchronization is
	# needed when you access non local variables.
	print(peerId, channel, variableName, value);

hg = Homegear("/var/run/homegear/homegearIPC.sock", eventHandler);
```

Please note that the callback method is called from a different thread. Please use thread synchronization when accessing shared variables.

To execute a RPC method, just type `hg.<method name>`. For example to set the system variable "TEST" to "6" and retrieve it again:

```
hg.setSystemVariable("TEST", 6);
print(hg.getSystemVariable("TEST"));
```

See Examply.py for a full example.

## Links

* [Homegear Website](https://homegear.eu)
* [Homegear Reference](https://ref.homegear.eu)
* [Homegear Documentation](https://doc.homegear.eu)