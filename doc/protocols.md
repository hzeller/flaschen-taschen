Protocols
=========

To make it simple to illuminate the matrix, there is a simple network protocol
that allows to stream images to the FlaschenTaschen installation (we used to
support some other protocols natively, but they moved out to be done via a
dedicated bridge).

## Flaschen Taschen Protocol

Receives UDP packets with a raw [PPM file][ppm] (`P6`) on port 1337 in a
single datagram per image.
A ppm file has a simple text header followed by the binary RGB image data.
We have another feature: 'offset'; since the header is already defined in a
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
8      # optional y offset
10     # optional z offset.
```

Upcoming, but not everywhere deployed yet (so: don't use yet unless you're
at Toorcamp currently talking to the Noisebridge installation), is the ability
to provide the offsets directly in a specially formatted header comment.

That way, it is possible to use PPM image packages that provide control over
a comment. The comment must start with exactly `#FT:`. In the following example,
the (x,y) offset is (5,8), the layer (z-offset) is 10:

```
P6     # Magic number
10 10  # width height (decimal, number in ASCII)
#FT: 5 8 10
255    # values per color (fixed). After newline, 10 * 10 * 3 bytes RGB data follows
```
![](../img/udp.png)<br/>

The optional offset determines where the image is displayed on the
Flaschen Taschen display relative to the top left corner (provided in the
[remote Flaschen Taschen class][remote-ft] as a `SetOffset(x, y, z)` method).

The (x,y) offset allows to place an image at an arbitrary position - good
for animations or having non-overlapping areas on the screen.

The z offset represents a layer above the background. Zero is the background
image, layers above that are overlaying content if there is a color set (black
in layers not the background are regarded transparent).
That way, it is possible to write games simply (consider typical arcade games
with slow moving background, a little faster moving middelground and a fast
moving character in the front. This allows for full screen sprites essentially).

Or you can overlay, say a message over the currently running content.
The nice thing is is that you don't need to know what the current background
is - a fully networked composite display essentially :)
Note: layers above the background (z=0) will auomatically turn transparent again
if they stick around for a while without being updated.

Since the server accepts a standard PPM format, sending an image is as
simple as this; you can make use of the convenient pnm-tools:

```bash
# This works in the bash shell providing the pseudo-files /dev/udp/host/port
bash$ jpegtopnm color.jpg | pnmscale -xysize 20 20 > /dev/udp/ft.noise/1337
# replace 'ft.noise' with 'localhost' if you have
# a locally running server, e.g. terminal based
```

If you're not using `bash`, then the `/dev/udp/...` path won't work, then
you can use the network-swiss army knife `socat`
```
$ jpegtopnm color.jpg | pnmscale -xysize 20 20 | socat STDIO UDP-SENDTO:ft.noise:1337
```

#### Notes
For huge displays (more than 21k pixels) we run into UDP packet size limit of
roughly 64k. In that case, you need to send several tiles of the image and use
the offset feature to place them on the screen.
(We might consider adding a TCP protocol for these cases).

Note, this is _not_ an issue with the large FlaschenTaschen installation at
Noisebridge (1575 pixels) as it requires less than 8% of the UDP limit. But it
could be an issue with huge assemblies using an
[RGB Matrix](../server#rgb-matrix-panel-display) with many small LEDs.

#### Implementations
You find more in the [client directory](../client) to directly send
content to the server, including a [convenient class][remote-ft] that provides
an implementation on the client side.

There is a [C++ API](../client/udp-flaschen-taschen.h) and
a [Python API](../client/flaschen.py) in that directory.

There is a [FlaschenTaschen VNC implementation](https://github.com/scottyallen/flaschenvnc) by Scotty. Scotty also wrote a server implementation for the ESP8266
that runs LED strips in the [Noise Square Table] and the Noisebridge library
shelves.

To watch videos, there is now a [FlaschenTaschen VLC output](https://git.videolan.org/?p=vlc.git;a=commit;h=cf334f257868d20b6a6ce024994e84ba3e3448c3) by François Revol available (until it hits distributions, you have to [compile VLC from git](https://wiki.videolan.org/UnixCompile/)).

### Network address of Installations
Within Noisebridge, the hostname of the large 45x35 pixel installation
is `ft.noise`.
The smaller 25x20 display is called `ftkleine.noise`.
Then there are pixel strip implementations for
the bookshelves with hostname `bookcase.noise` and
the [Noise Square Table], hostname `square.noise`.

[ppm]: http://netpbm.sourceforge.net/doc/ppm.html
[remote-ft]: ../client/udp-flaschen-taschen.h
[Noise Square Table]: https://noisebridge.net/wiki/Noise_Square_Table