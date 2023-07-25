
#include "videoreader.h"
#include <memory>
#include <iostream>
#include <chrono>
#include <thread>

#pragma comment(lib, "avcodec")
#pragma comment(lib, "avdevice")
#pragma comment(lib, "avfilter")
#pragma comment(lib, "avformat")
#pragma comment(lib, "avutil")
#pragma comment(lib, "swresample")
#pragma comment(lib, "swscale")
#pragma comment(lib, "SDL2")

#undef main

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    std::wcerr << "sdl2_ffmpeg_player.exe <filename>" << std::endl;
    return -1;
  }
  std::unique_ptr<VideoReader> videoReader = std::make_unique<VideoReader>();
  videoReader->start(argv[1]);
  while(1)
  {
    std::chrono::milliseconds duration(1000);
    std::this_thread::sleep_for(duration);
    if (videoReader->quitStatus())
    {
      break;
    }
  }

  std::wcout << "finished." << std::endl;
  return 0;
}
