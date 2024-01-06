
#include "videocontroller.h"
#include <iostream>

using namespace player;

VideoController::VideoController()
  : Observer()
{
  m_globalState = std::make_shared<GlobalState>();
  m_primaryVideoReader = std::make_unique<VideoReader>();
  m_primaryVideoReader->addObserver(this);
}

VideoController::~VideoController()
{
}

void VideoController::update(Subject* observer)
{
  // VideoReader
  {
    auto videoReader = static_cast<VideoReader*>(observer);
    if (videoReader)
    {
      m_globalState->clearVideoPacketRead();
      m_globalState->clearAudioPacketRead();
      std::cout << "VideoReader finished." << std::endl;
      m_isVideoFinished = true;
    }
  }
}

void VideoController::start(const std::string& filename)
{
  m_globalState->setup(filename);
  m_primaryVideoReader->start(m_globalState);
}


