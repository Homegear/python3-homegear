import time
from homegear import Homegear

# This callback method is called on Homegear variable changes
def eventHandler(peerId, channel, variableName, value):
	# Note that the event handler is called by a different thread than the main thread. I. e. thread synchronization is
	# needed when you access non local variables.
	print(peerId, channel, variableName, value);

hg = Homegear("/var/run/homegear/homegearIPC.sock", eventHandler);

while(not hg.connected()):
	time.sleep(1);

print(hg.logLevel());
print(hg.listDevices());

while(hg.connected()):
	time.sleep(1);
