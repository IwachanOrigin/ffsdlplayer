
#include "videocontroller.h"
#include <iostream>

using namespace player;

VideoController::VideoController()
  : Observer()
{
  // Primary
  m_primaryGlobalState = std::make_shared<GlobalState>();
  m_videoReader = std::make_unique<VideoReader>();
  m_videoReader->addObserver(this);
  m_videoDecoder = std::make_unique<VideoDecoder>();
  m_videoDecoder->addObserver(this);
  m_videoRenderer = std::make_unique<VideoRenderer>();
  m_videoRenderer->addObserver(this);

  // Secondary
  m_secondaryGlobalState = std::make_shared<GlobalState>();
}

VideoController::~VideoController()
{
}

void VideoController::update(Subject* subject)
{
  switch(subject->subjectType())
  {
    using enum player::SubjectType;
    case Reader:
    {
      auto videoReader = static_cast<VideoReader*>(subject);
      if (videoReader)
      {
        m_videoReader->deleteObserver(this);
        m_isVideoReaderFinished = true;
        std::cout << "VideoReader finished." << std::endl;

        // Setup Next file
        m_secondaryGlobalState->setup(m_movFileVec.at(1));
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
          m_videoDecoder->stop();
          m_videoDecoder->deleteObserver(this);
          std::cout << "VideoDecoder finished." << std::endl;

          // Setup next reader, decoder
          m_isVideoReaderFinished = false;
          m_videoReader->addObserver(this);
          m_videoReader->start(m_secondaryGlobalState);
          std::cout << "VideoReader started." << std::endl;
          m_videoDecoder->addObserver(this);
          m_videoDecoder->start(m_secondaryGlobalState);
          std::cout << "VideoDecoder started." << std::endl;
        }
      }
    }
    break;

    case Renderer:
    {
      auto videoRenderer = static_cast<VideoRenderer*>(subject);
      if (videoRenderer)
      {
        std::cout << "VideoRenderer finished." << std::endl;
        m_finishedFileCount++;
        if (m_finishedFileCount >= m_movFileVec.size())
        {
          m_isFinished = true;
          m_videoRenderer->deleteObserver(this);
          break;
        }
        m_videoRenderer->start(m_secondaryGlobalState);
        std::cout << "VideoRenderer started." << std::endl;
      }
    }
    break;

    case None:
    {
      //
    }
    break;
  }
}

void VideoController::start(std::vector<std::string_view>& filenames)
{
  std::chrono::milliseconds ms(20);

  m_movFileVec = filenames;
  m_primaryGlobalState->setup(m_movFileVec.at(0));
  std::this_thread::sleep_for(ms);

  m_videoReader->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_videoDecoder->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_videoRenderer->start(m_primaryGlobalState);
}


