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
