import socket
from tkinter import *
import random
import time

WIDTH  = 45
HEIGHT = 35

class Flaschen(object):
  '''A Framebuffer display interface that sends a frame via UDP.'''

  def __init__(self, host, port, width, height, layer=5, transparent=False):
    '''

    Args:
      host: The flaschen taschen server hostname or ip address.
      port: The flaschen taschen server port number.
      width: The width of the flaschen taschen display in pixels.
      height: The height of the flaschen taschen display in pixels.
      layer: The layer of the flaschen taschen display to write to.
      transparent: If true, black(0, 0, 0) will be transparent and show the layer below.
    '''
    self.width = width
    self.height = height
    self.layer = layer
    self.transparent = transparent
    self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self._sock.connect((host, port))
    header = ''.join(["P6\n",
                      "%d %d\n" % (self.width, self.height),
                      "255\n"]).encode('utf-8')
    footer = ''.join(["0\n",
                      "0\n",
                      "%d\n" % self.layer]).encode('utf-8')
    self._data = bytearray(width * height * 3 + len(header) + len(footer))
    self._data[0:len(header)] = header
    self._data[-1 * len(footer):] = footer
    self._header_len = len(header)
    self._footer_len = len(footer)
    self._screen_len = len(self._data)-self._header_len-self._footer_len

  def clear(self):
    for y in range(HEIGHT):
        for x in range(WIDTH):
            self.set(x,y, (255,0,0))
    self.send()
    # self._data[self._header_len+1:-self._footer_len] = ('0'*self._screen_len).encode('utf-8')

  def set(self, x, y, color):
    '''Set the pixel at the given coordinates to the specified color.

    Args:
      x: x offset of the pixel to set
      y: y offset of the piyel to set
      color: A 3 tuple of (r, g, b) color values, 0-255
    '''
    if x >= self.width or y >= self.height or x < 0 or y < 0:
      return
    if color == (0, 0, 0) and not self.transparent:
      color = (1, 1, 1)

    offset = (x + y * self.width) * 3 + self._header_len
    self._data[offset] = color[0]
    self._data[offset + 1] = color[1]
    self._data[offset + 2] = color[2]
  
  def send(self):
    '''Send the updated pixels to the display.'''
    self._sock.send(self._data)



def main() -> None:
    screen = Flaschen(host='ft.noise', port=1337, width=WIDTH, height=HEIGHT, layer=5, transparent=False)
    depth = [random.randint(0, 10) for _ in range(WIDTH)]
    yval = [0] * WIDTH
    while True:
        screen.clear()
        for x in range(WIDTH):
            yval[x] += random.randint(0, 2)
            for y in range(depth[x]):
                screen.set(x, (yval[x] + y) % HEIGHT, (200, 0, 0))
        screen.send()
        input("Press Enter to continue...")


if __name__ == "__main__":
    main()