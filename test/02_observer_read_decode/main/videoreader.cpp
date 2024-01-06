
#include <iostream>
#include <thread>
#include <chrono>

#include "videoreader.h"
#include "globalstate.h"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

using namespace player;

VideoReader::~VideoReader()
{
  this->stop();
}

int VideoReader::start(std::shared_ptr<GlobalState> gs)
{
  m_gs = gs;
  if (m_gs == nullptr)
  {
    return -1;
  }

  // start read thread
  std::thread([&](VideoReader *reader)
  {
    reader->readThread(m_gs);
  }, this).detach();

  return 0;
}

void VideoReader::stop()
{
  m_isFinished = true;
}

int VideoReader::readThread(std::shared_ptr<GlobalState> gs)
{
  int ret = -1;

  // retrieve global VideoState reference
  auto globalState = gs;

  // Set the AVFormatContext for the global videostate ref
  auto& formatCtx = globalState->inputFmtCtx();

  AVPacket* packet = av_packet_alloc();
  if (packet == nullptr)
  {
    std::cerr << "Could not alloc packet" << std::endl;
    return -1;
  }

  // Init stream index.
  auto& videoStreamIndex = m_gs->videoStreamIndex();
  auto& audioStreamIndex = m_gs->audioStreamIndex();

  // Init
  std::chrono::milliseconds delayms(10);

  // main decode loop. read in a packet and put it on the queue
  while (1)
  {
    // check audio and video packets queues size
    if (globalState->sizeAudioPacketRead() + globalState->sizeVideoPacketRead() > MAX_QUEUE_SIZE)
    {
      // !!!!!!!
      // !!!!!!! Delete code to future. !!!!!!!
      // !!!!!!!
      globalState->clearAudioPacketRead();
      globalState->clearVideoPacketRead();

      // wait for audio and video queues to decrease size
      std::this_thread::sleep_for(delayms);
      continue;
    }
    // read data from the AVFormatContext by repeatedly calling av_read_frame
    ret = av_read_frame(formatCtx, packet);
    if (ret < 0)
    {
      if (ret == AVERROR_EOF)
      {
        // !!!!!!!
        // !!!!!!! Delete code to future. !!!!!!!
        // !!!!!!!
        globalState->clearAudioPacketRead();
        globalState->clearVideoPacketRead();

        // wait for the rest of the program to end
        while (globalState->nbPacketsAudioRead() > 0 && globalState->nbPacketsVideoRead() > 0)
        {
          std::this_thread::sleep_for(delayms);
        }

        // media EOF reached, quit
        break;
      }
      else if (formatCtx->pb->error == 0)
      {
        // no read error, wait for user input
        std::this_thread::sleep_for(delayms);
        continue;
      }
      else
      {
        // exit for loop in case of error
        break;
      }
    }

    // put the packet in the appropriate queue
    if (packet->stream_index == videoStreamIndex)
    {
      globalState->pushVideoPacketRead(packet);
    }
    else if (packet->stream_index == audioStreamIndex)
    {
      globalState->pushAudioPacketRead(packet);
    }
    else
    {
      // Otherwise free the memory
      av_packet_unref(packet);
    }
  }

  // Notify
  this->notifyObservers();

  return 0;
}


