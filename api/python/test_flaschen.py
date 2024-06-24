from typing import Any

import numpy as np

import flaschen

UDP_IP = "localhost"
UDP_PORT = 1337

WIDTH = 45
HEIGHT = 35


def test_flaschen_init(benchmark: Any) -> None:
    benchmark(lambda: flaschen.Flaschen(UDP_IP, UDP_PORT, WIDTH, HEIGHT))


def test_send(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT, WIDTH, HEIGHT)
    benchmark(lambda: ft.send())


def test_set(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT, WIDTH, HEIGHT)
    benchmark(lambda: ft.set(0, 0, (1, 1, 1)))


def test_set_full(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT, WIDTH, HEIGHT)

    def test() -> None:
        for y in range(HEIGHT):
            for x in range(WIDTH):
                ft.set(x, y, (1, 1, 1))

    benchmark(test)


def test_send_array(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT)
    ones_array = np.ones((HEIGHT, WIDTH, 3), dtype=np.uint8)
    benchmark(lambda: ft.send_array(ones_array, (0, 0, 0)))


def test_send_array_transparent(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT, transparent=True)
    ones_array = np.ones((HEIGHT, WIDTH, 3), dtype=np.uint8)
    benchmark(lambda: ft.send_array(ones_array, (0, 0, 0)))


def test_send_new_array(benchmark: Any) -> None:
    ft = flaschen.Flaschen(UDP_IP, UDP_PORT)
    benchmark(
        lambda: ft.send_array(np.ones((HEIGHT, WIDTH, 3), dtype=np.uint8), (0, 0, 0))
    )
