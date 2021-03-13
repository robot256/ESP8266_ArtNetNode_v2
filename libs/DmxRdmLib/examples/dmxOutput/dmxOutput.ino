/*
dmxInput - a simple example for espDMX library
Copyright (c) 2016, Matthew Tong
https://github.com/mtongnz/espDMX

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/
*/

#include <DmxRdmLib.h>

#define LED_PIN 1   // define an analog Output pin

byte* myBuffer;  // define a buffer variable


void setup()
{
	// allocating dmx data memory block
	myBuffer = (byte*)os_malloc(sizeof(byte) * 512);
	memset(myBuffer, 0, 512);  // set every buffer value to value 0

	// Start dmxA, direction of RS485 driver on pin 5 
	dmxA.begin(5, myBuffer);
}


void loop()
{
	// count up the first 100 channels 
	for (int i = 0; i < 100; i++)
	{
		myBuffer[i]++;
	}

	// handle 
	dmxA.handler();
}
