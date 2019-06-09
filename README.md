python3-homegear
================

python3-homegear is a python extension to connect to Homegear over Unix Domain Sockets. It supports all of Homegear's RPC methods and live event reception.

## Prerequisites

The extension requires `libhomegear-ipc` to be installed and it needs at least Python version 3. To install it, add the Homegear APT repository for your distribution (see https://homegear.eu/downloads.html) and execute

```bash
apt install libhomegear-ipc
```

Alternatively on non Debian-like systems you can compile libhomegear-ipc manually:

```bash
git clone https://github.com/Homegear/libhomegear-ipc
cd libhomegear-ipc
./makeRelease.sh
```

## Setup

if you have pip, just do:

```bash
sudo python3 -m pip install homegear
```

To compile and install the extension manually, download it from GibHut and execute

```bash
sudo python3 setup.py install
```

## Methods

There is only one object available: `Homegear`. It takes two parameters in it's constructor: The path to the Homegear IPC socket (normally `/var/run/homegear/homegearIPC.sock`) and a callback method. The callback method is executed when a device variable is updated in Homegear. On instantiation the class waits until it is connected succesfully to Homegear. After 2 seconds it returns even if there is no connection. To check, if the object is still connected, you can call `connected()`. Apart from this method, you can call all RPC methods available in Homegear ([see ref.homegear.eu](https://ref.homegear.eu/rpc.html)).

## Behaviour on no connection

When there is no connection to Homegear, the constructor returns after 2 seconds. It indefinitely tries to reconnect until it is able to establish a connection. The same happens on connection loss. To check if the module is connected, call `connected()`. Even when there is no connection, you can still call all RPC methods without exception. The return value will be `None`.

## Type conversion

### Python variable to Homegear variable

Python | Homegear
-------|---------
None | Void
Bool | Boolean
Long | Integer
Float | Float
Unicode | String
Bytes | Binary
List | Array
Tuple | Array
Dict | Struct

### Homegear variable to Python variable

Homegear | Python
-------|---------
Void | None
Boolean | Bool
Integer | Long
Float | Float
String | Unicode
Binary | Bytes
Array | List
Struct | Dict

## Usage example

A minimal example:

```python
from homegear import Homegear

# This callback method is called on Homegear variable changes
def eventHandler(eventSource, peerId, channel, variableName, value):
	# Note that the event handler is called by a different thread than the main thread. I. e. thread synchronization is
	# needed when you access non local variables.
	print("Event handler called with arguments: source: " + eventSource + " peerId: " + str(peerId) + "; channel: " + str(channel) + "; variable name: " + variableName + "; value: " + str(value));

hg = Homegear("/var/run/homegear/homegearIPC.sock", eventHandler);
```

Please note that the callback method is called from a different thread. Please use thread synchronization when accessing shared variables.

To execute a RPC method, just type `hg.<method name>`. For example to set the system variable "TEST" to "6" and retrieve it again:

```python
hg.setSystemVariable("TEST", 6);
print(hg.getSystemVariable("TEST"));
```

A full example:

```python
import time
from homegear import Homegear

# This callback method is called on Homegear variable changes
def eventHandler(eventSource, peerId, channel, variableName, value):
	# Note that the event handler is called by a different thread than the main thread. I. e. thread synchronization is
	# needed when you access non local variables.
	print("Event handler called with arguments: source: " + eventSource + " peerId: " + str(peerId) + "; channel: " + str(channel) + "; variable name: " + variableName + "; value: " + str(value));

hg = Homegear("/var/run/homegear/homegearIPC.sock", eventHandler);

# hg waits until the connection is established (but for a maximum of 2 seonds).

hg.setSystemVariable("TEST", 6);
print("getSystemVariable(\"TEST\") after setting \"TEST\" to 6: ", hg.getSystemVariable("TEST"));

hg.setSystemVariable("TEST", ["One", 2, 3.3]);
print("getSystemVariable(\"TEST\") after setting \"TEST\" to an array: ", hg.getSystemVariable("TEST"));

hg.setSystemVariable("TEST", {"One": 1, 2: "Two", 3: [3, 3, 3]});
print("getSystemVariable(\"TEST\") after setting \"TEST\" to a struct: ", hg.getSystemVariable("TEST"));

counter = 0;
while(hg.connected()):
	time.sleep(1);
	counter += 1;
	hg.setSystemVariable("TEST", counter);
```

## Links

* [GitHub Project](https://github.com/Homegear/python3-homegear)
* [Homegear Website](https://homegear.eu)
* [Homegear Reference](https://ref.homegear.eu)
* [Homegear Documentation](https://doc.homegear.eu)

