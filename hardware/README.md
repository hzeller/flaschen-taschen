16 LED strip Raspberry Pi Adapter
=================================

Adapter to connect up to 16 SPI-type LED strips (e.g. WS2801 or LDP6803). The output pins
contain GND, CLK and Data. Also, 5V is given which can be useful for active termination of
longer wires.

Pinout of each of the connectors:

GND | CLK | DATA | +5V
---:|:---:|:----:|:---

(GND is pointing towards the Rasbperry Pi GPIO header).

### Features

* 16 data outputs for LED strips including GND and 5V.
* Input power (5V) operates level shifter as well as the Rasbpeery Pi; no separate USB supply
  needed.
* Fused power output to prevent accidents.
* Breakout for I2C, UART and SPI busses of Raspberry Pi to allow connecting other peripherals.

The CLK is shared on the input, but each connector gets a dedicated buffered signal.

The 5V output, accessible on each of the 16 connectors, is fused, so that accidental shorts when
messing with the data lines in the field do not cause harm. Use a fast 1A fuse.

(The outputs are beefy enough that they even could support a couple of shorter LED strips directly,
in that case use up to 20A fuse. Generally it is advised to separately power the strips though).

[ As soon as design is tested in the field, there will be an OshPark link here ]

![](../img/pi-adapter-sch.png)

![](../img/pi-adapter-pcb.png)
