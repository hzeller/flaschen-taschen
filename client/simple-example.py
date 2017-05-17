import flaschen

UDP_IP = 'localhost'
UDP_PORT = 1337

ft = flaschen.Flaschen(UDP_IP, UDP_PORT, 45, 35)

while True:
  for b in xrange(0, 256):
    for y in xrange(0, ft.height):
      for x in xrange(0, ft.width):
        ft.set(x, y, ((x * 255) / 45, (y * 255) / 35, b))
    ft.send()
