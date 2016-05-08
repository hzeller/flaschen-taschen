Flaschen Taschen Clients
========================

Programs to send content to the FlaschenTaschen server. The server implements
multiple protocols so that it is easy to use various means to connect to
FlaschenTaschen; see toplevel
directory [README.md](../README.md#getting-pixels-on-flaschen-taschen).

This directory provides:
  * `send-text` binary, that prints a static or scrolling text.
  * `send-image` binary, that reads an arbitrary image (including
    animated *.gifs), scales it and sends to FlaschenTaschen.
  * `send-video` binary, that reads an arbitrary video, scales it and
    sends to FlaschenTaschen.
  * A [simple-example.cc](./simple-example.cc) and [simple-animation.cc](./simple-animation.cc)
    code example.

### Network destination

The clients connect to the display over the network. The
default hostname is pointing to the installation within Noisebridge
(currently `ft.noise`).

You can change that with commandline flags (e.g. `send-text`, `send-image`,
and `send-video` all have a `-h <host>` option) or via the environment
variable `FT_DISPLAY`.

So if you are working with a particular instance of FlaschenTaschen (e.g.
a [local terminal](../server/README.md#terminal)), just set the environment
variable for ease of playing.

```
export FT_DISPLAY=localhost
```

## Send-Text

### Compile
```bash
make
```

### Use
```
usage: ./send-text [options] <TEXT>
Options:
        -g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 45x<font-height>+0+0+1
        -l <layer>      : Layer 0..15. Default 1 (note if also given in -g, then last counts)
        -h <host>       : Flaschen-Taschen display hostname.
        -f <fontfile>   : Path to *.bdf font file
        -s<ms>          : Scroll milliseconds per pixel (default 60). 0 for no-scroll.
        -o              : Only run once, don't scroll forever.
        -c<RRGGBB>      : Text color as hex (default: FFFFFF)
        -b<RRGGBB>      : Background color as hex (default: 000000)
        -v              : Scroll text vertically 
```

Sample
```bash
./send-text -f fonts/6x10.bdf "We ♥ Flaschen Taschen"

# Or coordinated horizontal and verticalmessages
./send-text -h localhost "♥Flaschen" -f fonts/5x5.bdf -s 60 -g 45x35+0+15+15 & ./send-text -h localhost "Taschen " -f fonts/5x5.bdf  -v  -s 60 -g 45x35+20+0+15 && fg

# Or, how about showing the time
while : ; do sleep 1 ; ./send-text -f fonts/9x18.bdf -s0 `date +%H:%M` ; done

```


Text has a default layer of 1, so it is hovering above the background image.
If you don't want that, you can explicitly set it as last value in the geometry
specification.

## Send-Image

### Compile
```bash
# Need some devel libs
sudo apt-get install libgraphicsmagick++-dev libwebp-dev
make send-image
```

### Use
```
usage: ./send-image [options] <image>
Options:
        -g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 20x20+0+0+0
        -l <layer>      : Layer 0..15. Default 0 (note if also given in -g, then last counts)
        -h <host>       : Flaschen-Taschen display hostname.
        -s[<ms>]        : Scroll horizontally (optionally: delay ms; default 60).
        -C              : Just clear given area and exit.
```

Essentially just send the FlaschenTaschen display an image over the network:

```
./send-image -g10x20+15+7 some-image.png
```

Image will be scaled to the given size (here 10x20) and shown
on the FlaschenTaschen display at the given offset (here 15 pixels x-offset,
7 pixels y-offset).

The program exits as soon as the image is sent unless it is an animated gif in
which case `send-image` keeps streaming until interrupted with `Ctrl-C`.

If you want to scroll a long image accross the display, use the `-s` option. In
this case, only the height is scaled to the display height and the image is
scrolled infinitely over width.

Let's try this with an example image:

```
./send-image -s ../img/flaschen-taschen-black.ppm
```

## Send-Video

### Compile
```bash
# Need some devel libs
sudo apt-get install libavcodec-dev libavformat-dev libswscale-dev
make send-video
```

### Use
```
usage: ./send-video [options] <video>
Options:
        -g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 20x20+0+0
        -h <host>          : Flaschen-Taschen display hostname.
        -l <layer>         : Layer 0..15. Default 0 (note if also given in -g, then last counts)
        -v                 : verbose.
```

![](../img/ft-movie-night.jpg)

## Example Code

Coding content for FlaschenTaschen is trivial as you just need to send it UDP
packets with the content. Any language of your choice that supports networking
will do.

For C++, there is a simple implementation of such a 'client display', the
[simple-example.cc](./simple-example.cc) helps to get started.

```c++
#include "udp-flaschen-taschen.h"

#define DISPLAY_WIDTH  20
#define DISPLAY_HEIGHT 20

int main() {
    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket("ft.noise");
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    const Color red(255, 0, 0);
    canvas.SetPixel(0, 0, red);              // Sample with color variable.
    canvas.SetPixel(5, 5, Color(0, 0, 255)); // or just use inline (here: blue).

    canvas.Send();                           // Send the framebuffer.
}
```

Next step, try a [simple-animation.cc](./simple-animation.cc)
<a href="./simple-animation.cc"><img src="../img/invader.png" width="50px"></a>
