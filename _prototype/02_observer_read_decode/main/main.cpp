
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

  return 0;
}
