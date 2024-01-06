
#include "videocontroller.h"
#include <iostream>

using namespace player;

VideoController::VideoController()
  : Observer()
{
  m_globalState = std::make_shared<GlobalState>();
  m_primaryVideoReader = std::make_unique<VideoReader>();
  m_primaryVideoReader->addObserver(this);
  m_primaryVideoDecoder = std::make_unique<VideoDecoder>();
  m_primaryVideoDecoder->addObserver(this);
}

VideoController::~VideoController()
{
}

void VideoController::update(Subject* subject)
{
  // VideoReader
  {
    auto videoReader = static_cast<VideoReader*>(subject);
    if (videoReader)
    {
      std::cout << "VideoReader finished." << std::endl;
      m_isVideoFinished = true;
    }
  }

  // VideoDecoder
  {
    auto videoDecoder = static_cast<VideoDecoder*>(subject);
    if (videoDecoder)
    {
      if (m_isVideoFinished)
      {
        m_primaryVideoDecoder->stop();
        std::chrono::milliseconds ms(100);
        std::this_thread::sleep_for(ms);
        std::cout << "VideoDecoder finished." << std::endl;
      }
    }
  }
}

void VideoController::start(const std::string& filename)
{
  m_globalState->setup(filename);
  m_primaryVideoReader->start(m_globalState);
  m_primaryVideoDecoder->start(m_globalState);
}


