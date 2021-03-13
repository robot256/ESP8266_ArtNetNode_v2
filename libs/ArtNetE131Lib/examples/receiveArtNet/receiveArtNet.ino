/*
receiveArtNet - a simple example for espArtNetRDM library
Copyright (c) 2016, Matthew Tong
https://github.com/JonasArnold/ArtNetE131Lib_esp8266

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/

EXPLANATION:
This small example software outputs the value of channel "channelLed" on the pin defined with LED_PIN.
Also check the Net, Subnet, Universe Variables to ensure correct ArtNet communication.
*/

#include <ArtNetE131Lib.h>
#include <ESP8266WiFi.h>   // needed for MAC address only

#define LED_PIN 1   // define an analog Output pin

// ArtNet Definitions 
#define ARTNET_OEM 0x0123    // Artnet OEM Code
#define ESTA_MAN 0x08DD      // ESTA Manufacturer Code
#define ESTA_DEV 0xEE000000  // RDM Device ID (used with Man Code to make 48bit UID)

// Instanciate
espArtNetRDM myArtRdm;

// defining settings
IPAddress ipAddress = IPAddress(2, 0, 0, 100);    // Static IP of this device
IPAddress subnetMask = IPAddress(255, 0, 0, 0);   // Subnet Mask 
bool dhcpEnabled = false;						  // False = DHCP Disabled
char* shortName = "espArtNetNode1";			      // short name
char* longName = "ESP ArtNet Node 1";			  // full name
uint8_t macAddress[6];						      // MAC address of this device
bool htpMerge = true;

// ArtNet settings
uint8_t netNum = 0;
uint8_t subnetNum = 0;
uint8_t universeNum = 0;
uint8_t _group;
uint8_t _port;		      
uint8_t channelLed = 1;   // DMX Channel to control the LED

void dmxHandle(uint8_t group, uint8_t port, uint16_t numChans, bool syncEnabled)
{
	if (_group == group && _port == port)
	{
		// Output the value of channel 1 on the LED
		analogWrite(LED_PIN, myArtRdm.getDMX(_group, _port)[channelLed -1]);  // -1 because buffer of dmx data starts at 0
	}
}

void setup()
{
	// Get MAC Address
	WiFi.macAddress(macAddress);

	// Init Art RDM
	myArtRdm.init(ipAddress, subnetMask, dhcpEnabled, shortName, longName, ARTNET_OEM, ESTA_MAN, macAddress);

	// add group and port
	_group = myArtRdm.addGroup(netNum, subnetNum);
	_port = myArtRdm.addPort(_group, 0, universeNum, RECEIVE_DMX, htpMerge);  // DMX OUT = Receive DMX

	// set protocol to ARTNET
	myArtRdm.setProtocolType(_group, _port, ARTNET);

	// set callback function to be called at new dmx data
	myArtRdm.setArtDMXCallback(dmxHandle);

	// start listening to UDP packets
	myArtRdm.begin();

	// set to output mode => drives an LED
	pinMode(LED_PIN, OUTPUT);
}


void loop()
{
	myArtRdm.handler();
}
