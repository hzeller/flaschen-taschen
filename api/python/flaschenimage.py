# -*- mode: python; c-basic-offset: 4; indent-tabs-mode: nil; -*-
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://gnu.org/licenses/gpl-2.0.txt>

import socket
import io
import time
import _thread
from PIL import Image


class Flaschen(object):
    """ A Framebuffer display interface that sends a frame via UDP."""

    def __init__(self, host, port):
        """
        Args:
            host: The flaschen taschen server hostname or ip address.
            port: The flaschen taschen server port number.
        """
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.connect((host, port))
        self._stop = False
        self._thread = _thread
        self._nr_threads = 0

    def send(self, image, width=0, height=0, xoffset=0, yoffset=0, layer=0, timeout=0, ms_between_frames=0):
        """
        Send image to display.
        Args:
            image: PIL image to send
            width: The width of the flaschen taschen display in pixels.
            height: The height of the flaschen taschen display in pixels.
            xoffset: Offset of image in x-direction
            yoffset: Offset of image in y-direction
            layer: The layer of the flaschen taschen display to write to.
            timeout: duration of screening the image
            ms_between_frames: if animation this value defines
        """
        if width == 0:
            width = image.width

        if height == 0:
            height = image.height

        header = ("Q7\n%d %d\n255\n" % (width, height)).encode()
        header += ("%d\n %d\n %d\n" % (xoffset, yoffset, layer)).encode()
        self._stop = False

        self._thread.start_new_thread(self._send_loop, (image, header, timeout, ms_between_frames))

    def stop(self):
        self._stop = True

    def clear(self, layer):
        img = Image.new('RGB', (1024, 1024), color='black')
        self.send(img, layer=layer)

    def is_running(self):
        return bool(self._nr_threads > 0)

    def _send_loop(self, image, header, timeout=0, ms_between_frames=0):
        self._nr_threads += 1
        total_time_passed = 0
        start = time.time()
        is_animated = False
        animation_running = False
        n_frame = 0
        last_frame_start = 0

        if ms_between_frames == 0:
            ms_between_frames = 100

        try:
            is_animated = image.is_animated
            animation_running = True
        except AttributeError:
            # no animation
            pass

        while not self._stop and (total_time_passed <= timeout or animation_running):

            if is_animated:
                image.seek(n_frame)
                if n_frame == image.n_frames-1:
                    n_frame = -1
                    if total_time_passed > timeout:
                        animation_running = False

            image_bytes = io.BytesIO()
            image.save(image_bytes, 'png')

            data = header + image_bytes.getvalue()

            if last_frame_start != 0:
                last_frame_time_passed = time.perf_counter() - last_frame_start
                while last_frame_time_passed*1000 <= ms_between_frames:
                    time.sleep(0.01)
                    last_frame_time_passed = time.perf_counter() - last_frame_start

            self._sock.send(data)

            last_frame_start = time.perf_counter()
            total_time_passed = time.time() - start
            n_frame += 1

        self._nr_threads -= 1









