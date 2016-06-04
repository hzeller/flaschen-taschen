Server implementation
=====================

The server implements various [protocols](../doc/protocols.md) so that
it is easy to interface with the display over the network.

## Regular Flaschen Taschen server

For the actual Flaschen Taschen display, simply build with

```
  make
```

(Note, this needs the http://spixels.org library, which is included as git
sub-module. So you have to `git clone --recursive` the flaschen-taschen repo).

These are the options

```
usage: ./ft-server [options]
Options:
        -D <width>x<height> : Output dimension. Default 45x35
        -I <interface>      : Which network interface to wait for
                              to be ready (e.g eth0. Empty string '' for no waiting).
                              Default ''
        --layer-timeout <sec>: Layer timeout: clearing after non-activity (Default: 15)
        -d                  : Become daemon
        --pixel-pusher      : Run PixelPusher protocol (default: false)
        --opc               : Run OpenPixelControl protocol (default: false)
(By default, only the FlaschenTaschen UDP protocol is enabled)
```

```bash
 # Run as root.
 sudo ./ft-server -d --opc --pixel-pusher
```

To change resolution and composition of the actual display, you have to
manually adapt the code in main.cc (look for `display.AddColumn()`) - to
choose type of LED strip and to which SPI port on the spixels hardware they
are connected to.

The server has to be started as root as it has to access and initialize the
GPIO pins, but it drops privileges to uid=daemon, gid=daemon after that is done.

Stopping ? Kill the hard way `sudo killall ft-server`.

*This assumes to be running on the Raspberry Pi* as it needs to access the
[GPIO pins](../hardware) to talk to the LED strips.

![](../img/ft-completed.jpg)

## Testing setups

If you are not at Noisebridge right now but want to hack on applications
that use the display, you can use different ways to visualize the output with
the following applications.

These test-setups allow to simulate the display in its full resolution
supporting the same protocol.

### Terminal

On any development machine with a somewhat decent terminal that allows for
24Bit [ANSI color][term-color] (konsole, xterm or gnome-terminal on Linux;
xterm/X11 or iTerm2 >= 3.0 on OS X), this `ft-server` allows to display the
result in a terminal.

```bash
  # Build with
  make FT_BACKEND=terminal
```

```bash
  # Then run (and optionally pass the desired resolution)
  ./ft-server -D45x35
```

![](../img/terminal-screenshot.png)
<img src="../img/terminal-screenshot-nb.png" width="100px">


### RGB Matrix Panel Display

If you have an [RGB matrix][rgb-matrix] connected to a Pi, you can run that
as Flaschen Taschen backend:

```bash
  make FT_BACKEND=rgb-matrix
```

```bash
  # Then run with
  sudo ./ft-server -D45x35
```

[![RGB-MatrixImpl][ft-rgb-vid]](https://www.youtube.com/watch?v=XeJBcNevuJE)
(this is a larger 160x96 display).

This runs on a Raspberry Pi; see the
[documentation in the RGB-Matrix project][rgb-matrix]

[rgb-matrix]: https://github.com/hzeller/rpi-rgb-led-matrix
[ft-rgb-vid]: ../img/rgb-matrix-sample-vid.jpg
[term-color]: https://gist.github.com/XVilka/8346728
