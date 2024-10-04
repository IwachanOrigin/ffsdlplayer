
#include "globalstate.h"
#include <iostream>

using namespace player;

GlobalState::GlobalState()
{
  m_vs = std::make_unique<VideoState>();
  // For reader
  m_vs->audioPacketQueueRead.init();
  m_vs->videoPacketQueueRead.init();
  m_vs->flushPkt = av_packet_alloc();
  m_vs->flushPkt->data = (uint8_t*)"FLUSH";
  // For clock
  m_clockSyncType = SYNC_TYPE::AV_SYNC_AUDIO_MASTER;
}

GlobalState::~GlobalState()
{
  // For reader
  m_vs->audioPacketQueueRead.clear();
  m_vs->videoPacketQueueRead.clear();

  if (m_vs->audioCodecCtx)
  {
    avcodec_close(m_vs->audioCodecCtx);
    avcodec_free_context(&m_vs->audioCodecCtx);
    m_vs->audioCodecCtx = nullptr;
  }

  if (m_vs->videoCodecCtx)
  {
    avcodec_close(m_vs->videoCodecCtx);
    avcodec_free_context(&m_vs->videoCodecCtx);
    m_vs->videoCodecCtx = nullptr;
  }

  if (m_vs->decodeVideoSwsCtx)
  {
    sws_freeContext(m_vs->decodeVideoSwsCtx);
    m_vs->decodeVideoSwsCtx = nullptr;
  }

  if (m_vs->inputFmtCtx)
  {
    avformat_close_input(&m_vs->inputFmtCtx);
    avformat_free_context(m_vs->inputFmtCtx);
    m_vs->inputFmtCtx = nullptr;
  }

  if (m_vs->flushPkt)
  {
    av_packet_unref(m_vs->flushPkt);
    m_vs->flushPkt = nullptr;
  }
}

int GlobalState::setup(std::string_view filename)
{
  auto ret = avformat_open_input(&m_vs->inputFmtCtx, filename.data(), nullptr, nullptr);
  if (ret < 0)
  {
    std::cerr << "Could not open file " << filename << std::endl;
    return -1;
  }

  // reset streamindex
  m_vs->videoStreamIndex = -1;
  m_vs->audioStreamIndex = -1;

  // read packets of the media file to get stream info
  ret = avformat_find_stream_info(m_vs->inputFmtCtx, nullptr);
  if (ret < 0)
  {
    std::cerr << "Could not find stream info " << filename << std::endl;
    return -1;
  }

  // dump info about file onto standard error
  av_dump_format(m_vs->inputFmtCtx, 0, filename.data(), 0);

  // loop through the streams that have been found
  for (int i = 0; i < m_vs->inputFmtCtx->nb_streams; i++)
  {
    // look for the video stream
    if (m_vs->inputFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && m_vs->videoStreamIndex < 0)
    {
      m_vs->videoStreamIndex = i;
    }

    // look for the audio stream
    if (m_vs->inputFmtCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && m_vs->audioStreamIndex < 0)
    {
      m_vs->audioStreamIndex = i;
    }
  }

  // return with error in case no video stream was found
  if (m_vs->videoStreamIndex == -1)
  {
    std::cerr << "Could not open video stream" << std::endl;
    return -1;
  }

  ret = this->setupComponent(m_vs->videoStreamIndex);
  if (ret < 0)
  {
    std::cerr << "Could not setup video component." << std::endl;
    return -1;
  }

  // return with error in case no audio stream was found
  if (m_vs->audioStreamIndex == -1)
  {
    std::cerr << "Could not open audio stream" << std::endl;
    return -1;
  }

  ret = this->setupComponent(m_vs->audioStreamIndex);
  if (ret < 0)
  {
    std::cerr << "Could not setup audio component." << std::endl;
    return -1;
  }

  return 0;
}

// AVPacket - Read

int GlobalState::pushAudioPacketRead(AVPacket* packet)
{
  return m_vs->audioPacketQueueRead.push(packet);
}

int GlobalState::pushVideoPacketRead(AVPacket* packet)
{
  return m_vs->videoPacketQueueRead.push(packet);
}

int GlobalState::popAudioPacketRead(AVPacket* packet)
{
  int ret = m_vs->audioPacketQueueRead.pop(packet);
  return (packet == nullptr) ? -1 : ret;
}

int GlobalState::popVideoPacketRead(AVPacket* packet)
{
  int ret = m_vs->videoPacketQueueRead.pop(packet);
  return (packet == nullptr) ? -1 : ret;
}

// AVFrame - Decode

int GlobalState::pushVideoFrameDecoded(AVFrame* frame)
{
  return m_vs->videoFrameQueueDecoded.push(frame);
}

int GlobalState::pushAudioFrameDecoded(AVFrame* frame)
{
  return m_vs->audioFrameQueueDecoded.push(frame);
}

int GlobalState::popVideoFrameDecoded(AVFrame* frame)
{
  int ret = m_vs->videoFrameQueueDecoded.pop(frame);
  return (frame == nullptr) ? -1 : 0;
}

int GlobalState::popAudioFrameDecoded(AVFrame* frame)
{
  int ret = m_vs->audioFrameQueueDecoded.pop(frame);
  return (frame == nullptr) ? -1 : 0;
}

int GlobalState::setupComponent(const int& streamIndex)
{
  // check the given stream index in valid
  if (streamIndex < 0 || streamIndex >= m_vs->inputFmtCtx->nb_streams)
  {
    std::cerr << "invalid stream index" << std::endl;
    return -1;
  }

  // retrieve codec for the given stream index
  const AVCodec* codec = avcodec_find_decoder(m_vs->inputFmtCtx->streams[streamIndex]->codecpar->codec_id);
  if (codec == nullptr)
  {
    std::cerr << "unsupported codec" << std::endl;
    return -1;
  }

  // retrieve codec context
  AVCodecContext* codecCtx = avcodec_alloc_context3(codec);

  // use multi core
  // fhd, 60p = 1 threads
  // 4k, 60p  = 4 threads
  //codecCtx->thread_count = 4;
  //codecCtx->thread_type = FF_THREAD_FRAME;

  int ret = avcodec_parameters_to_context(codecCtx, m_vs->inputFmtCtx->streams[streamIndex]->codecpar);
  if (ret != 0)
  {
    std::cerr << "Could not copy codec context" << std::endl;
    return -1;
  }

  // init the AVCodecContext to use the given AVCodec
  if (avcodec_open2(codecCtx, codec, nullptr) < 0)
  {
    std::cerr << "unsupported codec" << std::endl;
    return -1;
  }

  switch (codecCtx->codec_type)
  {
    case AVMEDIA_TYPE_AUDIO:
    {
      m_vs->audioCodecCtx = std::move(codecCtx);
      m_vs->audioStream = m_vs->inputFmtCtx->streams[streamIndex];
    }
    break;

    case AVMEDIA_TYPE_VIDEO:
    {
      m_vs->videoCodecCtx = std::move(codecCtx);
      m_vs->videoStream = m_vs->inputFmtCtx->streams[streamIndex];
    }
    break;
  }
  return 0;
}

double GlobalState::masterClock()
{
  switch (m_avSyncType)
  {
    using enum SYNC_TYPE;
    case AV_SYNC_VIDEO_MASTER:
    {
      return this->calcVideoClock();
    }
    break;

    case AV_SYNC_AUDIO_MASTER:
    {
      return this->calcAudioClock();
    }
    break;

    case AV_SYNC_EXTERNAL_MASTER:
    {
      return this->calcExternalClock();
    }
    break;
  }

  std::cerr << "Error : Undefined a/v sync type" << std::endl;
  return -1;
}

double GlobalState::calcVideoClock()
{
  double delta = (av_gettime() - m_vs->videoDecodeCurrentPtsTime) / 1000000.0;
  return m_vs->videoDecodeCurrentPts + delta;
}

double GlobalState::calcAudioClock()
{
  double pts = m_vs->audioClock;
  int hwBufSize = m_vs->audioBufSize - m_vs->audioBufIndex;
  int bytesPerSec = 0;
  int n = 2 * m_vs->audioCodecCtx->ch_layout.nb_channels;

  if (m_vs->audioStream)
  {
    bytesPerSec = m_vs->audioCodecCtx->sample_rate * n;
  }

  if (bytesPerSec)
  {
    pts -= (double) hwBufSize / bytesPerSec;
  }

  return pts;
}

double GlobalState::calcExternalClock()
{
  m_externalClockTime = av_gettime();
  m_externalClock = m_externalClockTime / 1000000.0;

  return m_externalClock;
}

void GlobalState::streamSeek(const int64_t& pos, const int& rel)
{
  if (!m_seekReq)
  {
    m_seekPos = pos;
    m_seekFlags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
    m_seekReq = 1;
  }
}

