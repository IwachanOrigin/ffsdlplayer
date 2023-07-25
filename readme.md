
# ffsdlplayer

## Introduction

    This video player use to ffmpeg(v6.0+) and SDL2.  

## Dependency

    - SDL2 2.26.5  
    - FFMPEG 6.0+  

## Build

    - Ninja + LLVM 16.0  
powershell.exe cmake -S . -B build -G "\"Ninja Multi-Config"\" -D FFMPEG_PATH="/path/to/ffmpeg" -D SDL2_PATH="/path/to/sdl2"

