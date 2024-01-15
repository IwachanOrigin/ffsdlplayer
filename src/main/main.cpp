
#include "videocontroller.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace fs = std::filesystem;
using namespace player;

#undef main

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << argv[0] << " <Movie file directory path>" << std::endl;
    return -1;
  }

  // Set locale(use to the system default locale)
  std::wcout.imbue(std::locale(""));

  if (!fs::exists(argv[1]))
  {
    std::cerr << argv[1] << " is not found." << std::endl;
    return -1;
  }

  if (!fs::is_directory(argv[1]))
  {
    std::cerr << argv[1] << "is not directory." << std::endl;
    return -1;
  }

  // init SDL
  int ret = -1;
  ret = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
  if (ret != 0)
  {
    std::cerr << "Could not initialize SDL" << SDL_GetError() << std::endl;
    return -1;
  }

  std::vector<std::string> movieFileVec;
#ifdef _WIN32
  for (const auto& file : fs::directory_iterator(argv[1]))
  {
    auto attrs = GetFileAttributes(file.path().wstring().data());
    if ((attrs != INVALID_FILE_ATTRIBUTES) &&
      !(attrs & FILE_ATTRIBUTE_HIDDEN))
    {
      movieFileVec.push_back(file.path().string());
    }
  }
#else
  for (const auto& file : fs::directory_iterator(argv[1]))
  {
    if (file.path().filename().wstring().front() != '.')
    {
      movieFileVec.push_back(file.path().string());
    }
  }
#endif
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
