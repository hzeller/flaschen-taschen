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

 * Runs http://openpixelcontrol.org/ server on standard port 7890
 * Receives UDP packet on port 1337 interpreted as framebuffer.

[FlaschenTaschen]: https://noisebridge.net/wiki/Flaschen_Taschen
