Flaschen Taschen Clients
========================

Programs to send content to the FlaschenTaschen server. The server implements
multiple protocols so that it is easy to use various means to connect to
FlaschenTaschen; see toplevel
directory [README.md](../README.md#getting-pixels-on-flaschen-taschen).

This directory provides:
  * `send-image`, that reads an arbitrary image (including
    animated *.gifs), scales it and sends to the server via UDP.
  * (more TBD. Pull requests encouraged, hint hint...)

```bash
$ sudo aptitude install libmagick++-dev  # we need these devel libs
$ make
```

Example Code
============

The [simple-example.cc](./simple-example.cc) helps to get started.

```c++
#include "udp-flaschen-taschen.h"

#define DISPLAY_WIDTH  10
#define DISPLAY_HEIGHT 10

int main() {
    const int socket = OpenFlaschenTaschenSocket("flaschen-taschen.local");
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    Color red(255, 0, 0);
    canvas.SetPixel(0, 0, red);              // Sample with color variable.
    canvas.SetPixel(5, 5, Color(0, 0, 255)); // or just use inline (here: blue).

    canvas.Send();                           // Send the framebuffer.
}
```
