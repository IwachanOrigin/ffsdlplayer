
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
  m_primaryVideoReader->setSubjectMode(player::SubjectMode::Primary);
  m_primaryVideoDecoder = std::make_unique<VideoDecoder>();
  m_primaryVideoDecoder->addObserver(this);
  m_primaryVideoDecoder->setSubjectMode(player::SubjectMode::Primary);

  // Secondary
  m_secondaryGlobalState = std::make_shared<GlobalState>();
  m_secondaryVideoReader = std::make_unique<VideoReader>();
  m_secondaryVideoReader->setSubjectMode(player::SubjectMode::Secondary);
  m_secondaryVideoDecoder = std::make_unique<VideoDecoder>();
  m_secondaryVideoDecoder->setSubjectMode(player::SubjectMode::Secondary);
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
        switch (videoReader->subjectMode())
        {
          using enum player::SubjectMode;
          case Primary:
          {
            m_primaryVideoReader->deleteObserver(this);
            m_isPrimaryVideoReaderFinished = true;
            std::cout << "Primary VideoReader finished." << std::endl;

            m_secondaryVideoReader->addObserver(this);
            m_secondaryGlobalState->setup(m_movFileVec.at(1));
            m_secondaryVideoReader->start(m_secondaryGlobalState);
            std::cout << "Secondary VideoReader started." << std::endl;
            m_isSecondaryVideoReaderStarted = true;
          }
          break;

          case Secondary:
          {
            m_secondaryVideoReader->deleteObserver(this);
            std::cout << "Secondary VideoReader finished." << std::endl;
            m_isSecondaryVideoReaderFinished = true;
          }
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
        switch (videoDecoder->subjectMode())
        {
          using enum player::SubjectMode;
          case Primary:
          {
            if (m_isPrimaryVideoReaderFinished)
            {
              m_primaryVideoDecoder->stop();
              m_primaryVideoDecoder->deleteObserver(this);
              std::cout << "Primary VideoDecoder finished." << std::endl;

              m_secondaryVideoDecoder->addObserver(this);
              while (!m_isSecondaryVideoReaderStarted)
              {
                std::chrono::milliseconds ms(10);
                std::this_thread::sleep_for(ms);
              }
              m_secondaryVideoDecoder->start(m_secondaryGlobalState);
              std::cout << "Secondary VideoDecoder started." << std::endl;
            }
          }
          break;

          case Secondary:
          {
            if (m_isSecondaryVideoReaderFinished)
            {
              m_secondaryVideoDecoder->stop();
              m_secondaryVideoDecoder->deleteObserver(this);
              std::cout << "Secondary VideoReader finished." << std::endl;
              // For the wait time when the secondary decode thread finish.
              std::chrono::milliseconds ms(100);
              std::this_thread::sleep_for(ms);
              m_isFinished = true;
            }
          }
          break;
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

void VideoController::start(std::vector<std::string_view>& filenames)
{
  m_movFileVec = filenames;
  m_primaryGlobalState->setup(m_movFileVec.at(0));
  m_primaryVideoReader->start(m_primaryGlobalState);
  std::cout << "Primary VideoReader started." << std::endl;
  m_primaryVideoDecoder->start(m_primaryGlobalState);
  std::cout << "Primary VideoDecoder started." << std::endl;
}


