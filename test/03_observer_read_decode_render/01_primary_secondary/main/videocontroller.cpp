
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
  m_primaryVideoRenderer = std::make_unique<VideoRenderer>();
  m_primaryVideoRenderer->addObserver(this);
  m_primaryVideoRenderer->setSubjectMode(player::SubjectMode::Primary);

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
      auto reader = static_cast<VideoReader*>(subject);
      if (reader)
      {
        m_startedReadFileCount++;

        // Check all files finished.
        if (m_startedReadFileCount >= m_movFileVec.size())
        {
          if (m_primaryVideoReader)
          {
            m_primaryVideoReader->deleteObserver(this);
            m_primaryVideoReader->stop();
            m_primaryVideoReader.reset();
          }
          if (m_secondaryVideoReader)
          {
            m_secondaryVideoReader->deleteObserver(this);
            m_secondaryVideoReader->stop();
            m_secondaryVideoReader.reset();
          }
          if (m_primaryGlobalState)
          {
            m_primaryGlobalState.reset();
          }
          if (m_secondaryGlobalState)
          {
            m_secondaryGlobalState.reset();
          }
          std::cout << "All VideoReader finished." << std::endl;
          break;
        }

        switch (reader->subjectMode())
        {
          using enum player::SubjectMode;
          case Primary:
          {
            m_primaryVideoReader->deleteObserver(this);
            m_primaryVideoReader->stop();
            m_primaryVideoReader.reset();
            std::cout << "Primary VideoReader finished." << std::endl;

            // Setup next file and reader
            std::chrono::milliseconds ms(20);
            m_secondaryGlobalState->setup(m_movFileVec.at(m_startedReadFileCount));
            std::this_thread::sleep_for(ms);

            m_secondaryVideoReader = std::make_unique<VideoReader>();
            m_secondaryVideoReader->setSubjectMode(player::SubjectMode::Secondary);
            m_secondaryVideoReader->addObserver(this);
            m_secondaryVideoReader->start(m_secondaryGlobalState);
            std::cout << "Secondary VideoReader started." << std::endl;
          }
          break;

          case Secondary:
          {
            m_secondaryVideoReader->deleteObserver(this);
            m_secondaryVideoReader->stop();
            m_secondaryVideoReader.reset();
            std::cout << "Secondary VideoReader finished." << std::endl;

            // Setup next file and reader
            std::chrono::milliseconds ms(20);
            m_primaryGlobalState->setup(m_movFileVec.at(m_startedReadFileCount));
            std::this_thread::sleep_for(ms);

            m_primaryVideoReader = std::make_unique<VideoReader>();
            m_primaryVideoReader->setSubjectMode(player::SubjectMode::Primary);
            m_primaryVideoReader->addObserver(this);
            m_primaryVideoReader->start(m_secondaryGlobalState);
            std::cout << "Primary VideoReader started." << std::endl;
          }
          break;
        }
      }
    }
    break;

    case Decoder:
    {
      auto decoder = static_cast<VideoDecoder*>(subject);
      if (decoder)
      {
        m_startedDecodeFileCount++;

        // Check all files finished.
        if (m_startedDecodeFileCount >= m_movFileVec.size())
        {
          if (m_primaryVideoDecoder)
          {
            m_primaryVideoDecoder->deleteObserver(this);
            m_primaryVideoDecoder->stop();
            m_primaryVideoDecoder.reset();
          }
          if (m_secondaryVideoDecoder)
          {
            m_secondaryVideoDecoder->deleteObserver(this);
            m_secondaryVideoDecoder->stop();
            m_secondaryVideoDecoder.reset();
          }
          std::cout << "All VideoDecoder finished." << std::endl;
          break;
        }

        switch (decoder->subjectMode())
        {
          using enum player::SubjectMode;
          case Primary:
          {
            m_primaryVideoDecoder->deleteObserver(this);
            m_primaryVideoDecoder->stop();
            m_primaryVideoDecoder.reset();
            std::cout << "Primary VideoDecoder finished." << std::endl;

            // Setup next decoder
            m_secondaryVideoDecoder = std::make_unique<VideoDecoder>();
            m_secondaryVideoDecoder->setSubjectMode(player::SubjectMode::Secondary);
            m_secondaryVideoDecoder->addObserver(this);
            m_secondaryVideoDecoder->start(m_secondaryGlobalState);
            std::cout << "Secondary VideoDecoder started." << std::endl;
          }
          break;

          case Secondary:
          {
            m_secondaryVideoDecoder->deleteObserver(this);
            m_secondaryVideoDecoder->stop();
            m_secondaryVideoDecoder.reset();
            std::cout << "Secondary VideoDecoder finished." << std::endl;

            // Setup next decoder
            m_primaryVideoDecoder = std::make_unique<VideoDecoder>();
            m_primaryVideoDecoder->setSubjectMode(player::SubjectMode::Primary);
            m_primaryVideoDecoder->addObserver(this);
            m_primaryVideoDecoder->start(m_primaryGlobalState);
            std::cout << "Primary VideoDecoder started." << std::endl;
          }
          break;
        }
      }
    }
    break;

    case Renderer:
    {
      auto renderer = static_cast<VideoRenderer*>(subject);
      if (renderer)
      {
        m_startedRenderFileCount++;

        // Check all files finished.
        if (m_startedRenderFileCount >= m_movFileVec.size())
        {
          if (m_primaryVideoRenderer)
          {
            m_primaryVideoRenderer->deleteObserver(this);
            m_primaryVideoRenderer->stop();
            m_primaryVideoRenderer.reset();
          }
          if (m_secondaryVideoRenderer)
          {
            m_secondaryVideoRenderer->deleteObserver(this);
            m_secondaryVideoRenderer->stop();
            m_secondaryVideoRenderer.reset();
          }
          std::cout << "All VideoDecoder finished." << std::endl;
          m_isFinished = true;
          break;
        }

        switch (renderer->subjectMode())
        {
          using enum player::SubjectMode;
          case Primary:
          {
            m_primaryVideoRenderer->deleteObserver(this);
            m_primaryVideoRenderer->stop();
            m_primaryVideoRenderer.reset();
            m_primaryGlobalState.reset();
            m_primaryGlobalState = std::make_shared<GlobalState>();
            std::cout << "Primary VideoRenderer finished." << std::endl;

            // Setup next renderer
            m_secondaryVideoRenderer = std::make_unique<VideoRenderer>();
            m_secondaryVideoRenderer->setSubjectMode(player::SubjectMode::Secondary);
            m_secondaryVideoRenderer->addObserver(this);
            m_secondaryVideoRenderer->start(m_secondaryGlobalState);
            std::cout << "Secondary VideoRenderer started." << std::endl;
          }
          break;

          case Secondary:
          {
            m_secondaryVideoRenderer->deleteObserver(this);
            m_secondaryVideoRenderer->stop();
            m_secondaryVideoRenderer.reset();
            m_secondaryGlobalState.reset();
            m_secondaryGlobalState = std::make_shared<GlobalState>();
            std::cout << "Secondary VideoRenderer finished." << std::endl;

            // Setup next renderer
            m_primaryVideoRenderer = std::make_unique<VideoRenderer>();
            m_primaryVideoRenderer->setSubjectMode(player::SubjectMode::Primary);
            m_primaryVideoRenderer->addObserver(this);
            m_primaryVideoRenderer->start(m_primaryGlobalState);
            std::cout << "Primary VideoRenderer started." << std::endl;
          }
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

void VideoController::start(std::vector<std::string>& filenames)
{
  std::chrono::milliseconds ms(100);

  m_movFileVec = filenames;
  m_primaryGlobalState->setup(m_movFileVec.at(m_startedReadFileCount));
  std::this_thread::sleep_for(ms);

  m_primaryVideoReader->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_primaryVideoDecoder->start(m_primaryGlobalState);
  std::this_thread::sleep_for(ms);

  m_primaryVideoRenderer->start(m_primaryGlobalState);
}


