# -*- mode: python; c-basic-offset: 2; indent-tabs-mode: nil; -*-
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

  def __init__(self, host, port, width, height, layer=0, transparent=False):
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
                      "255\n"])
    footer = ''.join(["0\n",
                      "0\n",
                      "%d\n" % self.layer])
    self._data = bytearray(width * height * 3 + len(header) + len(footer))
    self._data[0:len(header)] = header
    self._data[-1 * len(footer):] = footer
    self._header_len = len(header)

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
