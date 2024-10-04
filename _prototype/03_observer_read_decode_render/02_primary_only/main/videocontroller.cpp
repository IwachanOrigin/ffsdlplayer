
#include "videocontroller.h"
#include <iostream>

using namespace player;

VideoController::VideoController()
  : Observer()
{
  // Primary
  m_primaryGlobalState = std::make_shared<GlobalState>();
  m_primaryVideoReader = std::make_unique<VideoReader>();
  m_primaryVideoReader->addObserver(this);
  m_primaryVideoDecoder = std::make_unique<VideoDecoder>();
  m_primaryVideoDecoder->addObserver(this);
  m_primaryVideoRenderer = std::make_unique<VideoRenderer>();
  m_primaryVideoRenderer->addObserver(this);

  // Secondary
  m_secondaryGlobalState = std::make_shared<GlobalState>();
  m_secondaryVideoReader = std::make_unique<VideoReader>();
  m_secondaryVideoReader->addObserver(this);
  m_secondaryVideoDecoder = std::make_unique<VideoDecoder>();
  m_secondaryVideoDecoder->addObserver(this);
  m_secondaryVideoRenderer = std::make_unique<VideoRenderer>();
  m_secondaryVideoRenderer->addObserver(this);

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
        if (m_startedReadFileCount == 0)
        {
          m_primaryVideoReader->stop();
          m_primaryVideoReader->deleteObserver(this);
          m_startedReadFileCount++;
          std::cout << "Primary VideoReader finished." << std::endl;

          // Setup Next file and reader
          std::chrono::milliseconds ms(20);
          m_secondaryGlobalState->setup(m_movFileVec.at(1));
          std::this_thread::sleep_for(ms);

          m_secondaryVideoReader->addObserver(this);
          m_secondaryVideoReader->start(m_secondaryGlobalState);
          std::cout << "Secondary VideoReader started." << std::endl;

          break;
        }

        if (m_startedReadFileCount == 1)
        {
          m_startedReadFileCount++;
        }

        // Check all files finished.
        if (m_startedReadFileCount >= m_movFileVec.size())
        {
          m_secondaryVideoReader->deleteObserver(this);
          m_secondaryVideoReader->stop();
          std::cout << "Secondary VideoReader finished." << std::endl;
          break;
        }
      }
    }
    break;

    case Decoder:
    {
      auto videoDecoder = static_cast<VideoDecoder*>(subject);
      if (videoDecoder)
      {
        if (m_startedDecodeFileCount == 0)
        {
          m_primaryVideoDecoder->stop();
          m_primaryVideoDecoder->deleteObserver(this);
          m_startedDecodeFileCount++;
          std::cout << "Primary VideoDecoder finished." << std::endl;

          // Setup next decoder
          m_secondaryVideoDecoder->addObserver(this);
          m_secondaryVideoDecoder->start(m_secondaryGlobalState);
          std::cout << "Secondary VideoDecoder started." << std::endl;

          break;
        }

        if (m_startedDecodeFileCount == 1)
        {
          m_startedDecodeFileCount++;
        }


        if (m_startedDecodeFileCount >= m_movFileVec.size())
        {
          m_secondaryVideoDecoder->deleteObserver(this);
          m_secondaryVideoDecoder->stop();
          std::cout << "Secondary VideoDecoder finished." << std::endl;
          break;
        }
      }
    }
    break;

    case Renderer:
    {
      auto videoRenderer = static_cast<VideoRenderer*>(subject);
      if (videoRenderer)
      {
        if (m_startedRenderFileCount == 0)
        {
          m_primaryVideoRenderer->stop();
          m_primaryVideoRenderer->deleteObserver(this);
          m_startedRenderFileCount++;
          std::cout << "Primary VideoRenderer finished." << std::endl;

          m_secondaryVideoRenderer->addObserver(this);
          m_secondaryVideoRenderer->start(m_secondaryGlobalState);
          std::cout << "Secondary VideoRenderer started." << std::endl;

          break;
        }

        if (m_startedRenderFileCount == 1)
        {
          m_startedRenderFileCount++;
        }

        if (m_startedRenderFileCount >= m_movFileVec.size())
        {
          m_secondaryVideoRenderer->deleteObserver(this);
          m_secondaryVideoRenderer->stop();
          std::cout << "Secondary VideoRenderer finished." << std::endl;
          m_isFinished = true;
          break;
        }
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
  std::chrono::milliseconds ms(50);

  m_movFileVec = filenames;
  m_primaryGlobalState->setup(m_movFileVec.at(0));
  std::this_thread::sleep_for(ms);

  m_primaryVideoReader->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_primaryVideoDecoder->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_primaryVideoRenderer->start(m_primaryGlobalState);
}


