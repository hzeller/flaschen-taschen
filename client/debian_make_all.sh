#!/bin/bash

set -euo pipefail

make clean
sudo apt-get install -y graphicsmagick-libmagick-dev-compat libavcodec-dev libavformat-dev libswscale-dev
make -k send-text send-image send-video
