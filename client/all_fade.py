'''Fade all the Flaschens in the space together through all the colors.'''

import colorsys
import time

import bookcase
import flaschen
import multiflaschen

PORT = 1337
DELAY = 1.0/120
STEP = 1
LAYER = 10

ftkleine = flaschen.Flaschen('ftkleine.noise', PORT, 25, 25, layer=LAYER)
ft = flaschen.Flaschen('ft.noise', PORT, 45, 35, layer=LAYER)
bkft = flaschen.Flaschen('bookcase.noise', PORT, 810, 1, layer=LAYER)
noiseft = flaschen.Flaschen('square.noise', PORT, 291, 1, layer=LAYER)
bk = bookcase.Bookcase(bkft)

multi = multiflaschen.MultiFlaschen()
multi.add(bk, 0, 0)
multi.add(ft, bk.width, 0)
multi.add(noiseft, bk.width + ft.width, 0)
multi.add(ftkleine, bk.width + ft.width + noiseft.width, 0)

def hsv(h, s, v):
  rgb = colorsys.hsv_to_rgb(h/255.0, s/255.0, v/255.0)
  return tuple([int(x * 255.0) for x in rgb])

white = (255, 255, 255)

while True:
  for hue in xrange(0, 256, STEP):
    for x in xrange(multi.width):
      for y in xrange(multi.height):
        multi.set(x, y, hsv(hue, 255, 255))
    multi.send()
    time.sleep(DELAY)
