Server implementation
=====================

The server implements the protocols described in the
[toplevel README](../README.md).

## Regular Flaschen Taschen server

Just build with
```
  make
```

These are the options

```
usage: ./ft-server [options]
Options:
        -I <interface>      : Which network interface to wait for
                              to be ready (Empty string '' for no waiting).Default 'eth0'
        -d                  : Become daemon
```

```bash
# Run as root.
sudo ./ft-server -d
```

Stopping ? Kill the hard way `sudo killall ft-server`.

*This assumes to be running on the Raspberry Pi* as it needs to access the
GPIO pins to talk to the LED strips.

## Testing setups

While the actual FlaschenTaschen display is still work in progress (until
we have all the bottles, crates and LEDs), we already want to write applications
that use the display. For that it is of course desirable to see how it will
look like on the final.

For this, we have two possible test-setups, that allow to test the display in
its full final resolution and supporting the full protocol.

### Terminal

On any development machine with a somewhat decent terminal (konsole, xterm or
newer versions of gnome-terminal), this allows to display the result in a
terminal.

```bash
  # Build with
  make clean && make FLASCHEN_TASCHEN_BACKEND=terminal
```

```bash
# Then just run with
./ft-server -D45x35
```

![](../img/terminal-screenshot.png)

### RGB Display

If you have an [RGB matrix][rgb-matrix], you can use that with

```
  make clean && make FLASCHEN_TASCHEN_BACKEND=rgb-matrix
```

```
# Then just run with
sudo ./ft-server -D45x35
```

This runs on a Raspbeery Pi; see the
[documentation in the RGB-Matrix project][rgb-matrix]

[rgb-matrix]: https://github.com/hzeller/rpi-rgb-led-matrix
