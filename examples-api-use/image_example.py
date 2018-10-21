from libs import flaschenimage
import time
from PIL import Image

FLaImg = flaschenimage.Flaschen('localhost', 1337)

im = Image.open("image.png")

FLaImg.send(im)
FLaImg.clear(0)

gif_image = Image.open("image2.gif")
gif_image2 = Image.open("image2.gif")
gif_image3 = Image.open("image2.gif")

FLaImg.send(gif_image, height=64, width=64, layer=5, timeout=10, ms_between_frames=200)
FLaImg.send(gif_image2, xoffset=64, height=64, width=64, layer=5, timeout=10, ms_between_frames=100)
FLaImg.send(gif_image3, xoffset=0, height=64, width=128, layer=7, timeout=10, ms_between_frames=100)

while FLaImg.is_running():
    time.sleep(0.10)