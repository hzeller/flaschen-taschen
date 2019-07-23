#!/bin/bash

set -euo pipefail

make clean
sudo apt-get install -y graphicsmagick-libmagick-dev-compat libavcodec-dev libavformat-dev libswscale-dev
make send-text
make send-image
make send-video
