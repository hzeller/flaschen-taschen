# -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

import socket

class Flaschen(object):
  '''A Framebuffer display interface that sends a frame via UDP.'''

  def __init__(self, host, port, width, height, layer=0):
    self.width = width
    self.height = height
    self.layer = layer
    self.pixels = bytearray(width * height * 3)
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    self.sock.connect((host, port))
    self._header = ''.join(["P6\n",
                            "%d %d\n" % (self.width, self.height),
                            "255\n"])
    self._footer = ''.join(["0\n",
                            "0\n",
                            "%d\n" % self.layer])

  def set(self, x, y, color, transparent=False):
    '''Set the pixel at the given coordinates to the specified color.

    Args:
      x: x offset of the pixel to set
      y: y offset of the piyel to set
      color: A 3 tuple of (r, g, b) color values, 0-255
      transparent: If true, black(0, 0, 0) will be transparent and show the layer below.
    '''
    if x >= self.width or y >= self.height or x < 0 or y < 0:
      return
    if color == (0, 0, 0) and not transparent:
      color = (1, 1, 1)

    offset = (x + y * self.width) * 3
    self.pixels[offset] = color[0]
    self.pixels[offset + 1] = color[1]
    self.pixels[offset + 2] = color[2]
  
  def send(self):
    '''Send the updated pixels to the display.'''
    data = self._header + self.pixels + "\n" + self._footer
    self.sock.send(data)
