
#include "videocontroller.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace player;

#undef main

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << argv[0] << " <Movie File Path>" << std::endl;
    return -1;
  }

  VideoController vc;
  vc.start(argv[1]);
  std::chrono::milliseconds ms(100);
  while(!vc.isVideoFinished())
  {
    std::this_thread::sleep_for(ms);
  }

  return 0;
}
