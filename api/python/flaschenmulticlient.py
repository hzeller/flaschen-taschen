import flaschen


class FlaschenMultiClient(object):
    def __init__(self, host, port, width, height, client_count, layer=0):
        self.width = width
        self.height = height

        self.client_count = client_count
        self.client_array = []
        for client in range(0, self.client_count):
            self.client_array.append(flaschen.Flaschen(host, port, width / self.client_count, height,
                                                       client * width / self.client_count, 0, layer, True))

    def send_image(self, img):
        if img.width != self.width or img.height != self.height:
            raise Exception("image has to be the same size as the display")

        off = int(self.width / self.client_count)
        img = img.convert('RGBA')
        px = img.load()

        for x in range(0, off):
            for y in range(0, self.height):
                for client in range (0, self.client_count):
                    self.client_array[client].set(x, y, px[x + client * off, y])

        for client in range(0, self.client_count):
            self.client_array[client].send()
