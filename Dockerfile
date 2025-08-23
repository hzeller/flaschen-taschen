FROM ubuntu:14.04

RUN apt-get update && apt-get install -y \
  libgraphicsmagick++-dev \
  libwebp-dev \
  netpbm \
  socat \
  build-essential

# Big one
ENV FT_DISPLAY=ft.noise
# Small one
#ENV FT_DISPLAY=ftkleine.noise

WORKDIR /code/client/
