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

void dmxA_InputCallback(uint16_t num)
{
	byte* dataIn = dmxA.getChans();

	// => do something with the data here
	analogWrite(LED_PIN, dataIn[0]); // set the LED's brightness to the value of channel 1
}


void setup() {

	// set to output mode => drives an LED
	pinMode(LED_PIN, OUTPUT);

	// Start dmxA, direction of RS485 driver on pin 5 
	dmxA.begin(5);
	dmxA.dmxIn(true);  // set to input
	dmxA.setInputCallback(dmxA_InputCallback);
}


void loop()
{

}
