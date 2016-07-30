import time

import flaschen
from flaschen import hsv

UDP_IP = 'square.noise'
UDP_PORT = 1337
NUM_LEDS = 291
MAX_FPS = 10

ft = flaschen.Flaschen(UDP_IP, UDP_PORT, NUM_LEDS, 1)

def fill_rainbow(ft, num, initial_hue, deltahue):
  h = initial_hue;
  v = 255;
  s = 240;
  for i in xrange(0, num):
    rgb = hsv(h, s, v)
    ft.set(i, 0, rgb);
    h = (h + deltahue) % 255;

while True:
  for gHue in xrange(0, 256, 10):
    fill_rainbow(ft, NUM_LEDS, gHue, 7);
    ft.send()
    time.sleep(1.0 / MAX_FPS)
