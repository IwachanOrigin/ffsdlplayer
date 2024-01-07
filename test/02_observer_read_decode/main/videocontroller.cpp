
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
  switch(auto subjectType = subject->subjectType(); subjectType)
  {
    using enum player::SubjectType;
    case Reader:
    {
      auto videoReader = static_cast<VideoReader*>(subject);
      if (videoReader)
      {
        std::cout << "VideoReader finished." << std::endl;
        m_isVideoReaderFinished = true;
      }
    }
    break;

    case Decoder:
    {
      auto videoDecoder = static_cast<VideoDecoder*>(subject);
      if (videoDecoder)
      {
        if (m_isVideoReaderFinished)
        {
          m_primaryVideoDecoder->stop();
          std::chrono::milliseconds ms(100);
          std::this_thread::sleep_for(ms);
          m_isFinished = true;
          std::cout << "VideoDecoder finished." << std::endl;
        }
      }
    }
    break;

    case Renderer:
    {
      //
    }
    break;

    case None:
    {
      //
    }
    break;
  }
}

void VideoController::start(std::string_view filename)
{
  m_globalState->setup(filename);
  m_primaryVideoReader->start(m_globalState);
  m_primaryVideoDecoder->start(m_globalState);
}


