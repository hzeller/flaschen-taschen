from flaschenQ7client import FlaschenQ7Client
import time
from PIL import Image

flaQ7client = FlaschenQ7Client('localhost', 1337)

width = 256
height = 96
test_image = Image.new('RGB', (width, height), color='red')

# 256 x 96 x 3 = 73728 > ~65kB Packet limit
# The image wouldn't be possible to send with original flaschen taschen client
# This example uses the Q7 format client, which converts the image internally to png before sending via udp

flaQ7client.send(test_image, layer=5, timeout=10, ms_between_frames=100)

while flaQ7client.is_running():
    time.sleep(0.10)
