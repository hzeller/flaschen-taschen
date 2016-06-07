'''Clear all layers in the given flaschen taschen.

This is helpful when a layer gets stuck because someone is writing to part of a layer, but something still
remains on part of the layer they're not writing to.
'''
import sys

import flaschen

UDP_PORT = 1337

def main(argv):
  hostname = 'ft.noise'
  if len(sys.argv) > 1:
    hostname = sys.arg[1]
  black = (0, 0, 0)
  for layer in xrange(17):
    ft = flaschen.Flaschen(hostname, UDP_PORT, 45, 35, layer=layer)
    for y in xrange(0, 35):
      for x in xrange(0, 45):
        ft.set(x, y, black, transparent=True)
    ft.send()

if __name__ == '__main__':
  argv = gflags.FLAGS(sys.argv)
  main(argv)
