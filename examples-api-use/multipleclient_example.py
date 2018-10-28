from flaschenmulticlient import FlaschenMultiClient
import time
from PIL import Image

width = 256
height = 96
clients_count = 2
timeout = 10

test_image = Image.new('RGB', (width, height), color='red')

# 256 x 96 x 3 = 73728 > ~65kB Packet limit
# The image wouldn't be possible to send with original flaschen taschen client.
# This example uses the wrapper class multipleserver.py which creates internally multiple flaschen taschen clients for
# sending the image in several pieces.
# The image will be sliced horizontally, e.g. if the image has a the size of 256x96 and clients_count = 2, 2 parts
# with the size of 128 x 96 will be transmitted.

flaschen_multi_server = FlaschenMultiClient('localhost', 1337, width, height, clients_count)

start = time.time()
while True:
    flaschen_multi_server.send_image(test_image)
    time.sleep(0.1)

    if time.time() - start > timeout:
        break

