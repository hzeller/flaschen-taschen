Hardware
========

## Connecting LED strips to the Pi

LED strips are controlled by a Raspberry Pi, connected via
a custom level shifter that first has been developed for FlaschenTaschen
but is now broken out as [separate Spixels PCB and library][spixels].

This adapter to the Raspberry Pi can support up to 16 SPI LED strips (e.g. WS2801 or APA102) at once.

<a href="https://github.com/hzeller/spixels/tree/master/hardware"><img src="../img/pi-adapter-pcb.png" width="300px"></a>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
<a href="http://youtu.be/HAbR64yrjUk"><img src="../img/spixels-video.jpg"></a>


## Pi Mounting Plate

We have the Pi in a separate crate together with one of the power supplies. It is
held in place with zip-ties through the holes of the crate using this
laser-cut mounting plate. To be easily modifyable,
[it is written in PostScript](./pi-mounting-rig.ps) and can be converted to a
[DXF file](./pi-mounting-rig.dxf) (a typical input to laser cutters)
with the Makefile.

![](../img/pi-mounting-rig.png)

[spixels-hardware]: https://github.com/hzeller/spixels/tree/master/hardware
[spixels]: http://spixels.org/
