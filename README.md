Noisebridge Flaschen Taschen Project
====================================

Server for [FlaschenTaschen].

Current hardware setup:

 * Top Display: *WS2811* strip to GPIO 18 (pin 12)
 * Bottom Display: *LPD6803* strip: clk on GPIO 17 (pin 11); data on GPIO 11 (pin 23)
 
```
$ git clone --recursive https://github.com/hzeller/flaschen-taschen.git
$ cd flaschen-taschen
$ make
$ sudo ./ft-server
```

The Flaschen-Taschen server `ft-server` becomes a daemon (also it drops
privileges to daemon:daemon). Kill the hard way with `sudo killall ft-server`

 * Runs http://openpixelcontrol.org/ server on standard port 7890
 * Receives UDP packet on port 1337 interpreted as framebuffer.

Within noisebridge, the name is `flaschen-taschen.local`.

So, for instance you can send a raw image to the service like this; each pixel
represented by a red/green/blue byte.

```
cat raw-image.bytes | socat STDIO UDP-SENDTO:flaschen-taschen.local:1337
```
The current display is 10x10 pixels, so it would be 3 * 100 bytes.

[FlaschenTaschen]: https://noisebridge.net/wiki/Flaschen_Taschen
