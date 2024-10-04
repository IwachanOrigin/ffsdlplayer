
#include <iostream>
#include <chrono>
#include <thread>

#undef main

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cout << argv[0] << " <Movie File Path>" << std::endl;
    return -1;
  }

  return 0;
}
