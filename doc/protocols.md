Protocols
=========

To make it simple to illuminate the matrix, there are _three_ protocols that
are all supported:

### Standard Flaschen Taschen protocol

Receives UDP packets with a raw [PPM file][ppm] (`P6`) on port 1337 in a
single datagram per image.
A ppm file has a simple text header followed by the binary RGB image data.
We add another feature: 'offset'; since the header is already defined in a
fixed way, we add a footer after the binary image data to be backward compatible.

A 10x10 image looks like this (header + data + optional footer). End-of-line
comments in the header are allowed with `#` character:

```
P6     # Magic number
10 10  # width height (decimal, number in ASCII)
255    # values per color (fixed). After newline, 10 * 10 * 3 bytes RGB data follows
```
![](../img/udp.png)<br/>
```
5      # optional x offset
5      # optional y offset
```

The optional offset determines where the image is displayed on the
Flaschen Taschen display relative to the top left corner (provided in the
[remote Flaschen Taschen class][remote-ft] as a `SetOffset(x, y)` method).

This allows to place an image at an arbitrary position - good for animations
or having non-overlapping areas on the screen.

Since the server accepts a standard PPM format, sending an image is as
simple as this:

```bash
# This works in the bash shell
cat image.ppm > /dev/udp/flaschen-taschen.local/1337   # ft-installation
# replace 'flaschen-taschen.local' with 127.0.0.1 if you have
# a locally running server, e.g. terminal based
```

If you're not using `bash`, then the `/dev/udp/...` path won't work, then
you can use the network-swiss army knife `socat`
```
cat image.ppm | socat STDIO UDP4-SENDTO:flaschen-taschen.local:1337
```

You find more in the [client directory](../client) to directly send
content to the server, including a [convenient class][remote-ft] that provides
an implementation on the client side.

### OpenPixelControl

The http://openpixelcontrol.org/ socket listens on standard port 7890
(Simulated layout row 0: left-right, row 1: right-left, row 2: left-right...;
this is what their standard `wall.py` script assumes).

![](../img/opc.png)

### PixelPusher

Provides pixel pusher control via standard beacon (this is cool to be used
together with processing, there are libs that support it).
(Simulated layout: 10 strips starting on the left with 10 pixels each;
pretty much like a standard framebuffer).

![](../img/pixelpusher.png)

### Installation
Within Noisebridge, the hostname is `flaschen-taschen.local`.

[ppm]: http://netpbm.sourceforge.net/doc/ppm.html
[remote-ft]: ../client/udp-flaschen-taschen.h
