import time
from homegear import Homegear

# This callback method is called on Homegear variable changes
def eventHandler(peerId, channel, variableName, value):
	# Note that the event handler is called by a different thread than the main thread. I. e. thread synchronization is
	# needed when you access non local variables.
	print("Event handler called with arguments: peerId: " + str(peerId) + "; channel: " + str(channel) + "; variable name: " + variableName + "; value: " + str(value));

hg = Homegear("/var/lib/homegear/homegearIPC.sock", eventHandler);

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