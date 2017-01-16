Protocols
=========

To make it simple to illuminate the matrix, there is a simple network protocol
that allows to stream images to the FlaschenTaschen installation (we used to
support some other protocols natively, but they moved out to be done via a
dedicated bridge).

## Flaschen Taschen Protocol

Receives **UDP** packets with a raw [PPM file][ppm] (`P6`) on port 1337 in a
single datagram per image.
A ppm file has a simple text header followed by the binary RGB image data.

FlaschenTaschen has a special feature that allows you to choose the offset the
image is displayed on the screen in (x,y) direction and as a layer.

To provide this additional information without violating the PPM file standard,
there is a special comment that you can put in the header (because `#`-comments
are allowed in the PPM header) that describe the offset of the image on the
display in X, Y and Z-direction (=layer).

The comment must start with exactly **`#FT:`**. In the following example, a 10x10
image is sent with the (x,y) offset of (5,8), the layer (z-offset) is 13:

```
P6     # Magic number
10 10  # width height (decimal, number in ASCII)
#FT: 5 8 13
255    # values per color (fixed). After newline, 10 * 10 * 3 bytes RGB data follows
```
![](../img/udp.png) (300 bytes RGB data)<br/>

(the header is followed by the binary image data, three bytes per pixel RGB, so in this case 10x10x3 = 300 bytes)

There is an alternative way to provide the offset by appending it at the _end_ of
the image data in a similar format like the header because that is sometimes
easier to do depending on your implementation.
Appending it at the end makes it also backward compatible with
standard PPM (as PPM just ignores additional data at the end).

Here the same 10x10 image with (header + data + optional footer). The footer is
similar formatted like the header: decimal numbers, separated by space).
End-of-line comments in the header/footer are allowed with `#` character:

```
P6     # Magic number
10 10  # width height (decimal, number in ASCII)
255    # values per color (fixed). After newline, 10 * 10 * 3 bytes RGB data follows
```
![](../img/udp.png) (300 bytes RGB data)<br/>
```
5      # optional x offset
8      # optional y offset
13     # optional z offset.
```

The optional offset determines where the image is displayed on the
Flaschen Taschen display relative to the top left corner (provided in the
[remote Flaschen Taschen class][cpp-client-api] as a `SetOffset(x, y, z)` method).

### Offsets, how do they work ?

The **(x,y) offset** allows to place an image at an arbitrary position on the
screen - good for animations or having non-overlapping areas on the screen.

The **z offset** represents a **layer** above the background. Zero is the
background image, layers above that are overlaying content if there is a
color set (black
in layers not the background are regarded transparent).
That way, it is possible to write games simply (consider typical arcade games
with slow moving background, a little faster moving middelground and a fast
moving character in the front. This allows for full screen sprites essentially).

Or you can overlay, say a message over the currently running content.
The nice thing is is that you don't need to know what the current background
is - a fully networked compositing display essentially :)
This allows display access for independent people on the network, right the
way we use it at Noisebridge.

Note: layers above the background (z=0) will auomatically turn transparent again
if they stick around for a while without being updated (that is
the `--layer-timeout` flag to the server).

### Send images right from the command-line

Since the server accepts a standard PPM format, sending an image is as
simple as this; you can make use of the convenient pnm-tools right from your
command line, how cool is that ?

```bash
# This works in the bash shell providing the pseudo-files /dev/udp/host/port
bash$ jpegtopnm color.jpg | stdbuf -o64k pnmscale -xysize 20 20 > /dev/udp/ft.noise/1337
# replace 'ft.noise' with 'localhost' if you have
# a locally running server, e.g. terminal based
```

If you're not using `bash`, then the `/dev/udp/...` path won't work, then
you can use the network-swiss army knife `socat`
```
$ jpegtopnm color.jpg | stdbuf -o64k pnmscale -xysize 20 20 | socat -b64000 STDIO UDP-SENDTO:ft.noise:1337
```

#### Implementations
You find some tools in the [`client/` directory](../client) to directly send
content to the server.

For the API level, there are a couple APIs provided in the [`api/`](../api)
directory, that let's you get right into it:

 * [C++ API][cpp-client-api]
 * [Python API][py-client-api]

For instance, the [FlaschenTaschen demos] by Carl are using the C++ API.

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

#### Notes
For huge displays (more than 21k pixels) we run into UDP packet size limit of
roughly 64k. In that case, you need to send several tiles of the image and use
the offset feature to place them on the screen.
(We might consider adding a TCP protocol for these cases).

Note, this is _not_ an issue with the large FlaschenTaschen installation at
Noisebridge (1575 pixels) as it requires less than 8% of the UDP limit. But it
could be an issue with huge assemblies using an
[RGB Matrix](../server#rgb-matrix-panel-display) with many small LEDs.

[ppm]: http://netpbm.sourceforge.net/doc/ppm.html
[cpp-client-api]: ../api/include/udp-flaschen-taschen.h
[py-client-api]: ../api/python/flaschen.py
[Noise Square Table]: https://noisebridge.net/wiki/Noise_Square_Table
[FlaschenTaschen demos]: https://github.com/cgorringe/ft-demos