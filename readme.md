
# ffsdlplayer

## Introduction

    This video player use to ffmpeg(v6.0+) and SDL2.  

## Dependency

    - SDL2 2.26.5  
    - FFMPEG 6.0+  

## Setup

### For Windows


### For Ubuntu

    sudo apt-get install libsdl2-dev  
    sudo apt-get install libsdl2-image-dev libsdl2-mixer-dev libsdl2-net-dev libsdl2-ttf-dev  

## Build

    - Ninja + LLVM 16.0  
powershell.exe cmake -S . -B build -G "\"Ninja Multi-Config"\" -D FFMPEG_PATH="/path/to/ffmpeg" -D SDL2_PATH="/path/to/sdl2"

