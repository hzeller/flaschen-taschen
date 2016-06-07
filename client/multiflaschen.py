from collections import defaultdict

class MultiFlaschen(object):
  '''A collection of multiple Flaschens that are addressed as a single unified Flaschen.

  Allows multiple Flaschens to be mapped onto a single 2d coordinate space, and treated a single large
  Flaschen.
  '''

  def __init__(self):
    self._2dmapping = defaultdict(lambda: defaultdict(lambda: (None, None, None)))
    self._fts = []
    self.width = 0
    self.height = 0

  def add(self, ft, x_offset, y_offset):
    '''

    Args:
      ft: A Flaschen object.
      x_offset: The horizontal offset to place this Flaschen at in the unified 2d coordinate space.
      y_offset: The vertical offset to place this Flaschen at in the unified 2d coordinate space.
    '''
    for x in xrange(0, ft.width):
      for y in xrange(0, ft.height):
        self._2dmapping[x + x_offset][y + y_offset] = (ft, x, y)
    self._fts.append(ft)
    self.width = max(self.width, x_offset + ft.width)
    self.height = max(self.height, y_offset + ft.height)

  def set(self, x, y, color):
    ft, xft, yft = self._2dmapping[x][y]
    if ft is not None:
      ft.set(xft, yft, color)

  def send(self):
    for ft in self._fts:
      ft.send()
  
  def clear(self):
    for ft in self._fts:
      ft.clear()
