/*
dmxChaser - a simple example for espDMX library
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

// 10 steps with 10 channels in each
byte dmxChase[][10] = { { 255, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 255, 0, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 255, 0, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 255, 0, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 255, 0, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 255, 0, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 255, 0, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 255, 0, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0, 255, 0 },
                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 255},
                        { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 },
                        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
};

void setup() 
{
	// Start dmxB, direction of RS485 driver on pin 6
	dmxB.begin(6);
}


void loop()
{
	// DMX Chaser
	for (int i = 0; i < 10; i++)
	{
		// Output channels 1 - 10 on dmxA
		dmxB.setChans(dmxChase[i], 10, 1);

		// 1 second between each step
		delay(1000);
	}
}
