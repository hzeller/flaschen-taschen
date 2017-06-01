import os
import png
import base64

from flask import *
from flaschen import *
app = Flask(__name__)

host = os.getenv("FT_HOST","localhost")
port = int(os.getenv("FT_PORT","1337"))
display_width = int(os.getenv("FT_WIDTH","45"))
display_height = int(os.getenv("FT_HEIGHT","35"))

flaschen_conn = Flaschen(host,port,display_width,display_height, layer=15)

@app.route('/data/<path:path>')
def static_file(path):
    return app.send_static_file(path)

@app.route('/ProcessCanvasData', methods=['POST'])
def process_canvas_data():
    datafile=request.files["data"]
    datablob=datafile.read()
    pngReader = png.Reader(bytes=datablob)
    (w,h,pixels,info) = pngReader.read()
    pixelLines = list(pixels)

    for row in range(0,h):
        for col in range(0,w):
            r = pixelLines[row][col*4+0]
            g = pixelLines[row][col*4+1]
            b = pixelLines[row][col*4+2]
            flaschen_conn.set(col, row, (r,g,b))    
    flaschen_conn.send()
    return ''

@app.route('/')
def ft_home():
    return render_template('FTCanvas.html',port=port,host=host,display_width=display_width,display_height=display_height)
