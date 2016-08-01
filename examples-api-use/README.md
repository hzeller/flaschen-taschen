Getting started with the API
============================

This gives you some starting point to get started with the API.

### Network destination

Your programs connect to the display over the network. If you give `NULL`
as the location in the `OpenFlaschenTaschenSocket()` API call, the
hostname is determined from the environment variable `FT_DISPLAY`. If that
is empty, it falls back to the Noisebridge installation `ft.noise`.

So if you are working with a particular instance of FlaschenTaschen (e.g.
a [local terminal](../server/README.md#terminal)), just set the environment
variable for ease of playing.

```
export FT_DISPLAY=localhost
```

## Example Code

Coding content for FlaschenTaschen is trivial as you just need to send it UDP
packets with the content. Any language of your choice that supports networking
will do.

For C++, there is a simple implementation of such a 'client display', the
[simple-example.cc](./simple-example.cc) helps to get started.

```c++
#include "udp-flaschen-taschen.h"

#define DISPLAY_WIDTH  45
#define DISPLAY_HEIGHT 35

int main() {
    // Open socket and create our canvas.
    const int socket = OpenFlaschenTaschenSocket("ft.noise");  // hostname.
    UDPFlaschenTaschen canvas(socket, DISPLAY_WIDTH, DISPLAY_HEIGHT);

    const Color red(255, 0, 0);
    canvas.SetPixel(0, 0, red);              // Sample with color variable.
    canvas.SetPixel(5, 5, Color(0, 0, 255)); // or just use inline (here: blue).

    canvas.Send();                           // Send the framebuffer.
}
```

Next step, try a [simple-animation.cc](./simple-animation.cc)
<a href="./simple-animation.cc"><img src="../img/invader.png" width="50px"></a>
