'''
Bookcase animation that lights up each square in the golden spiral design in sequence.

To run:

  python squares.py [hostname]
'''

import sys
import flaschen
import time
import bookcase

UDP_PORT = 1337
FPS = 15
DELAY = 1.0 / FPS
STEP = 20

def fade_segments(bk, segments, fade_time):
  bk.clear()
  for c in xrange(255, 0, -1 * STEP):
    for i in segments:
      bk.fill_segment(i, (c, c, c))
    bk.ft.send()
    time.sleep(DELAY)

squares = [
    [2, 3, 16, 17, 18, 7],
    [5, 6, 18, 21, 20, 19],
    [4, 19, 12, 11, 10, 0],
    [10, 8, 9, 16, 1],
    [13, 14, 21, 17, 9],
    [12, 20, 14, 15],
]

def main(argv):
  udp_ip = 'bookcase.noise'
  if len(argv) > 1:
    udp_ip = argv[1]

  ft = flaschen.Flaschen(udp_ip, UDP_PORT, 810, 1)
  bk = bookcase.Bookcase(ft)

  while True:
    for square in squares:
      fade_segments(bk, square, 1)

if __name__ == '__main__':
  argv = sys.argv
  main(argv)
