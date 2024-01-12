
#include "videocontroller.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace player;

#undef main

int main(int argc, char* argv[])
{
  if (argc < 3)
  {
    std::cout << argv[0] << " <Movie File Path 1> <Movie File Path 2>" << std::endl;
    return -1;
  }

  // Set locale(use to the system default locale)
  std::wcout.imbue(std::locale(""));

  // init SDL
  int ret = -1;
  ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
  if (ret != 0)
  {
    std::cerr << "Could not initialize SDL" << SDL_GetError() << std::endl;
    return -1;
  }

  std::vector<std::string_view> movieFileVec;
  movieFileVec.push_back(argv[1]);
  movieFileVec.push_back(argv[2]);
  VideoController vc;
  vc.start(movieFileVec);
  std::chrono::milliseconds ms(100);
  while(!vc.isFinished())
  {
    std::this_thread::sleep_for(ms);
  }

  // Release
  SDL_VideoQuit();
  SDL_AudioQuit();
  SDL_Quit();

  return 0;
}
