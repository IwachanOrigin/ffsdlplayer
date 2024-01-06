
#include "globalstate.h"

using namespace player;

GlobalState::GlobalState()
{
  m_videoState = std::make_unique<VideoState>();
  // For reader
  m_videoState->audioPacketQueueRead.init();
  m_videoState->videoPacketQueueRead.init();
  m_videoState->flushPkt = av_packet_alloc();
  m_videoState->flushPkt->data = (uint8_t*)"FLUSH";
  // For clock
  m_clockSyncType = SYNC_TYPE::AV_SYNC_AUDIO_MASTER;
}

GlobalState::~GlobalState()
{
  // For reader
  m_videoState->audioPacketQueueRead.clear();
  m_videoState->videoPacketQueueRead.clear();
  // For Decoder
  m_videoState->videoFrameQueueDecoded.clear();

  if (m_videoState->audioCtx)
  {
    avcodec_close(m_videoState->audioCtx);
    avcodec_free_context(&m_videoState->audioCtx);
    m_videoState->audioCtx = nullptr;
  }

  if (m_videoState->videoCtx)
  {
    avcodec_close(m_videoState->videoCtx);
    avcodec_free_context(&m_videoState->videoCtx);
    m_videoState->videoCtx = nullptr;
  }

  if (m_videoState->swsCtx)
  {
    sws_freeContext(m_videoState->swsCtx);
    m_videoState->swsCtx = nullptr;
  }

  if (m_videoState->decodeAudioSwrCtx)
  {
    swr_free(&m_videoState->decodeAudioSwrCtx);
    m_videoState->decodeAudioSwrCtx = nullptr;
  }

  if (m_videoState->inputFmtCtx)
  {
    avformat_close_input(&m_videoState->inputFmtCtx);
    avformat_free_context(m_videoState->inputFmtCtx);
    m_videoState->inputFmtCtx = nullptr;
  }

  if (m_videoState->flushPkt)
  {
    av_packet_unref(m_videoState->flushPkt);
    m_videoState->flushPkt = nullptr;
  }
}

// AVPacket - Read

int GlobalState::pushAudioPacketRead(AVPacket* packet)
{
  return m_videoState->audioPacketQueueRead.push(packet);
}

int GlobalState::pushVideoPacketRead(AVPacket* packet)
{
  return m_videoState->videoPacketQueueRead.push(packet);
}

int GlobalState::popAudioPacketRead(AVPacket* packet)
{
  int ret = m_videoState->audioPacketQueueRead.pop(packet);
  return (packet == nullptr) ? -1 : ret;
}

int GlobalState::popVideoPacketRead(AVPacket* packet)
{
  int ret = m_videoState->videoPacketQueueRead.pop(packet);
  return (packet == nullptr) ? -1 : ret;
}

// Calculation master clock
double GlobalState::calcMasterClock()
{
  switch(m_clockSyncType)
  {
    case SYNC_TYPE::AV_SYNC_VIDEO_MASTER:
    {
      return this->calcVideoClock();
    }
    break;

    case SYNC_TYPE::AV_SYNC_AUDIO_MASTER:
    {
      return this->calcAudioClock();
    }
    break;

    case SYNC_TYPE::AV_SYNC_EXTERNAL_MASTER:
    {
      return this->calcExternalClock();
    }
    break;
  }
  return -1;
}

double GlobalState::calcVideoClock()
{
  double delta = (av_gettime() - m_videoState->videoDecodeCurrentPtsTime) / 1000000.0;
  return m_videoState->videoDecodeCurrentPts + delta;
}

double GlobalState::calcAudioClock()
{
  double pts = m_videoState->audioClock;
  int hw_buf_size = m_videoState->audioBufSize - m_videoState->audioBufIndex;
  int bytes_per_sec = 0;
  int n = 2 * m_videoState->audioCtx->ch_layout.nb_channels;

  if (m_videoState->audioStream)
  {
    bytes_per_sec = m_videoState->audioCtx->sample_rate * n;
  }

  if (bytes_per_sec)
  {
    pts -= (double)hw_buf_size / bytes_per_sec;
  }

  return pts;
}

double GlobalState::calcExternalClock()
{
  m_videoState->externalClockTime = av_gettime();
  m_videoState->externalClock = m_videoState->externalClockTime / 1000000.0;

  return m_videoState->externalClock;
}
