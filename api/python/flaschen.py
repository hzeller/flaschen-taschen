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

# Netpbm raw RGB header.
_HEADER_P6 = b"""\
P6
%(width)d %(height)d
255
"""

# Flaschen Taschen footer holding the offset.
_FOOTER_FT = b"""\
%(x)d
%(y)d
%(z)d
"""

# Netpbm header with Flaschen Taschen offset included.
_HEADER_P6_FT = b"""\
P6
%(width)d %(height)d
#FT: %(x)d %(y)d %(z)d
255
"""

class Flaschen(object):
  '''A Framebuffer display interface that sends a frame via UDP.'''

  def __init__(self, host, port, width=0, height=0, layer=5, transparent=False):
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
    header = _HEADER_P6 % {b"width": self.width, b"height": self.height}
    footer = _FOOTER_FT % {b"x": 0, b"y": 0, b"z": self.layer}
    self._data = bytearray(width * height * 3 + len(header) + len(footer))
    self._data[0:len(header)] = header
    self._data[-1 * len(footer):] = footer
    self._header_len = len(header)

  @property
  def __array_interface__(self):
    '''An array interface to directly access the framebuffer pixel data.
    
    Direct writes to pixel data will ignore the transparent parameter.
    '''
    return {
      "shape": (self.height, self.width, 3),
      "typestr": "|u1",
      "data": memoryview(self._data)[
        self._header_len : self._header_len + (self.height * self.width * 3)
      ],
      "version": 3,
    }

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

  def send_array(self, pixels, offset):
    # (numpy.typing.ArrayLike, tuple[int, int, int]) -> None
    '''Send an array of pixels to the given offset.

    Can be used as an alternative to the send method.
    The initial transparent option is respected.
    The initial width, height, and layer is ignored.

    Args:
      pixels: An array-like of RGB pixels, shaped as (height, width, RGB)
              Color values are expected to be from 0 to 255.
      offset: The (x, y, layer) offset.
              X and Y begin at the top-left pixel.
    '''
    import numpy as np
    array = np.asarray(pixels, dtype=np.uint8)
    if len(array.shape) != 3 or array.shape[2] != 3:
      raise TypeError(
        "Pixel array must be shape (height, width, 3), got %r" % (array.shape,)
      )
    if not self.transparent:
      array = array.copy(order="C")  # Prevent modifying the original array.
      array[(array == (0, 0, 0)).all(axis=-1)] = (1, 1, 1)

    header = _HEADER_P6_FT % {
      b"width": array.shape[1],
      b"height": array.shape[0],
      b"x": offset[0],
      b"y": offset[1],
      b"z": offset[2],
    }
    self._sock.send(header + array.tobytes())
