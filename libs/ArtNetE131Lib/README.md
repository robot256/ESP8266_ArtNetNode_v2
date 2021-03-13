# ArtNet V4 and sACN (E131) Library for esp8266
[![Build Status](https://travis-ci.com/JonasArnold/ArtNetE131Lib_esp8266.svg?branch=master)](https://travis-ci.com/JonasArnold/ArtNetE131Lib_esp8266)

With this library you can receive or send ArtNet V4 or sACN (E131) on an ESP8266.


Initial version by [Matthew Tong](https://github.com/mtongnz/ESP8266_ArtNetNode_v2/tree/master/libs/espArtnetRDM). The library was copied from there and extended.
The library was extended by utilising the [E131 Lib by forkineye](https://github.com/forkineye/E131).

## USAGE

** Note: ** The following library must be installed: [E131 by forkineye](https://github.com/forkineye/E131)

Instanciate:
```
  espArtNetRDM myArtRdm;
```

### Configuration:

Initialize:
```
myArtRdm.init(ArtNetOEMCode, EstaManufacturerCode, macAddress);
myArtRdm.init(shortName, ArtNetOEMCode, EstaManufacturerCode, macAddress);
myArtRdm.init(shortName, longName, ArtNetOEMCode, EstaManufacturerCode, macAddress);
myArtRdm.init(ipAddress, subnetMask, dhcpEnabled, ArtNetOEMCode, EstaManufacturerCode, macAddress);
myArtRdm.init(ipAddress, subnetMask, dhcpEnabled, shortName, longName, ArtNetOEMCode, EstaManufacturerCode, macAddress);
```
More information:
[ArtNet OEM](https://art-net.org.uk/join-the-club/oem-code-listing/),
[ESTA Manufacturer Code](http://tsp.esta.org/tsp/working_groups/CP/mfctrIDs.php),
[Get MAC address of ESP8266](https://techtutorialsx.com/2017/04/09/esp8266-get-mac-address/)

To add a ArtNet net and subnet:
```
uint8_t _group = myArtRdm.addGroup(netNum, subnetNum);  // store the return value
```

To add a Port (Universe):

`portType` can be:

- `DMX_OUT` = receive DMX from Network
- `DMX_IN` = send DMX to Network

`htpMerge`: `true` for Highest Takes Precedence, `false` for Latest Takes Precedence.

```
uint8_t _port = myArtRdm.addPort(_group, portNum, universeNum, portType, htpMerge);
```

To set the network protocol type:

`protocolType` can be:

- `ARTNET` = ArtNet V4
- `sACN_UNICAST` = sACN Unicast
- `sACN_MULTICAST` = sACN Multicast   **(not yet implemented)**

```
myArtRdm.setProtocolType(_group, _port, protocolType);  // store the return value
```

At the end of the configuration process start listening to UDP packets with this command:
```
myArtRdm.begin();
```

Place this function in main loop to let the library handle the data periodically:
```
myArtRdm.handler();
```


### Receiving ArtNet/sACN data:

Setup a callback function:
```
myArtRdm.setArtDMXCallback(dmxHandle);
```

and implement the callback function like this:
```
void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled)
{
	if(group == _group && port == _port)
	{
		// read out the value of the dmx channel (-1 because buffer of dmx data starts at 0)
		uint8_t valueOfDmxChannel = myArtRdm.getDMX(_group, _port)[channel -1];
	}
}
```


### Send Data to ArtNet/sACN:

To send data use the following function:
```
  myArtRdm.sendDMX(groupNum, portNum, broadcastAddress, dataArray, 512);
```



### General:
To set the Firmware version sent with each ArtPoll:
```
  myArtRdm.setFirmwareVersion(fwVersion);
```

To set the default ArtNet IP (generates IP by using the MAC address):
```
  myArtRdm.setDefaultIP();
```

