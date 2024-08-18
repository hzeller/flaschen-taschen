{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = with pkgs;
    [
       python3         # for Python API

       pkg-config
       graphicsmagick  # for send-image
       ffmpeg          # for send-video 
    ];
}
