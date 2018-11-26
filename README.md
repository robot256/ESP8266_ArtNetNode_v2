# EthernetDmxNode_esp8266
Forked from [mtongnz](https://github.com/mtongnz/ESP8266_ArtNetNode_v2)

The main goal of this version is to implement Multicast and Unicast sACN. In the original version by mtongnz only Multicast is implemented.
Fortunately [Shelby Merrick (Forkineye)](https://github.com/forkineye) already implemented both versions of sACN so it is only a question of the high-level implementation into the user interface and the setup part.

## Additional goals:
 - Use normal RGB LED for the Status LED.
 - Use normal LEDs for the DMX activity LEDs.
 - Beautify the code, make it more structured and clear.
 - I will not work with the WS2812 part of the node so it is possible I will **remove** this.
 - New modern user interface.
