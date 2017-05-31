from flaschen import *
import argparse

parser = argparse.ArgumentParser(description='Process some integers.')
parser.add_argument('-width', help='Display width', type=int, default=42)
parser.add_argument('-height', help='Display height', type=int, default=42)
parser.add_argument('-port', help='Server port', type=int, default=1337)
parser.add_argument('-host', help='Server host', default="localhost")
args = vars(parser.parse_args())

display_width = args["width"]
display_height = args["height"]
port = args["port"]
host = args["host"]
sq_size = 2

flaschen_conn = Flaschen(host,port,display_width,display_height)

for row in range(0,display_height):
    for col in range(0,display_width):
        if (int(row/sq_size)%2 ^ int(col/sq_size)%2):
            flaschen_conn.set(row, col, (255,0,0))
        else:
            flaschen_conn.set(row, col, (0,255,0))
flaschen_conn.send()

