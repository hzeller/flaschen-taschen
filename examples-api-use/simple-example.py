# (Hey Scotty, how can I set the 'include' path for python ?)
import os, sys, inspect
# realpath() will make your script run, even if you symlink it :)
cmd_folder = os.path.realpath(os.path.abspath(os.path.split(inspect.getfile( inspect.currentframe() ))[0]))
cmd_folder +=  '/../api/python'
if cmd_folder not in sys.path:
    sys.path.insert(0, cmd_folder)

import flaschen

UDP_IP = 'localhost'
UDP_PORT = 1337

ft = flaschen.Flaschen(UDP_IP, UDP_PORT, 45, 35)

while True:
  for b in range(0, 256):
    for y in range(0, ft.height):
      for x in range(0, ft.width):
        ft.set(x, y, ((x * 255) // 45, (y * 255) // 35, b))
    ft.send()
