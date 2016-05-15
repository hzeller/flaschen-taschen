class Bookcase(object):
  '''Flashen Taschen wrapper for the bookcase leds.

  This maps the bookcase leds onto a virtual 2d flaschen taschen that is the width and height of the bookcase.

  If you write to an (x, y) position that doesn't have an led, it will be ignored.
  '''

  def __init__(self, ft):
    self.ft = ft

    self.width = 106
    self.height = 173
    SEGMENT_LENGTHS = [
      41,
      26,
      105,
      105,
      40,
      65,
      66,
      106,
      10,
      14,
      24,
      5,
      10,
      4,
      10,
      9,
      25,
      15,
      65,
      40,
      10,
      15,
    ]

    self._segments = []

    s = 0
    for x in SEGMENT_LENGTHS:
      self._segments.append((s, x))
      s += x

    self._2dmapping = [[-1 for y in xrange(self.height)] for x in xrange(self.width)]
    self._set_horizontal(0, 0, [4, 5])
    self._set_horizontal(1, SEGMENT_LENGTHS[0], [10, 11, 12])
    self._set_horizontal(SEGMENT_LENGTHS[10] + 1, SEGMENT_LENGTHS[0] + SEGMENT_LENGTHS[8], [13, 14])
    self._set_horizontal(1, SEGMENT_LENGTHS[0] + SEGMENT_LENGTHS[1] - 1, [16, 17, 18])
    self._set_horizontal(0, SEGMENT_LENGTHS[6] + SEGMENT_LENGTHS[7] - 1, [3])

    self._set_vertical(0, 0, [0, 1, 2])
    self._set_vertical(SEGMENT_LENGTHS[10] + 1, SEGMENT_LENGTHS[0] + 1, [8, 9])
    self._set_vertical(SEGMENT_LENGTHS[10] + SEGMENT_LENGTHS[11], SEGMENT_LENGTHS[0] + 1, [15])
    self._set_vertical(SEGMENT_LENGTHS[4], 1, [19, 20, 21])
    self._set_vertical(SEGMENT_LENGTHS[3], 1, [6, 7])

  def _set_horizontal(self, x_offset, y_offset, segments):
    '''Add horizontal segments of leds to the 2d mapping.
    
    Args:
        x_offset: The horizontal offset to position the first segment at.
        y_offset: The vertical offset to position these segments at.
        segment: A list of segment ids to add to the 2d mapping.  They will be added one after another
            horizontally, starting at the first one specified
    '''
    length = sum([self._segments[s][1] for s in segments])
    start = self._segments[segments[0]][0]
    for i, x in enumerate(xrange(x_offset, x_offset + length)):
      self._2dmapping[x][y_offset] = start + i

  def _set_vertical(self, x_offset, y_offset, segments):
    '''Add vertical segments of leds to the 2d mapping.
    
    Args:
        x_offset: The horizontal offset to position these segments at.
        y_offset: The vertical offset to position the first segment at.
        segment: A list of segment ids to add to the 2d mapping.  They will be added one after another
            vertically, starting at the first one specified.'''
    length = sum([self._segments[s][1] for s in segments])
    start = self._segments[segments[0]][0]
    for i, y in enumerate(xrange(y_offset, y_offset + length)):
      self._2dmapping[x_offset][y] = start + i

  def set(self, x, y, color):
    '''Set a single pixel to the given color.

    All pixels are addressed in a full 2d grid based on their position on the bookshelf.
    '''
    i = self._2dmapping[x][y]
    if i == -1:
      return
    self.ft.set(i, 0, color)

  def fill_segment(self, segment_num, color):
    '''Set all the leds in the given segment number to the given color.

    A segment is any length of leds between the intersections with other led strips.
    '''
    offset, length = self._segments[segment_num]
    for x in xrange(offset, offset + length):
      self.ft.set(x, 0, color)

  def send(self):
    self.ft.send()
  
  def clear(self):
    self.ft.clear()
