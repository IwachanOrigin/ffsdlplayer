
#include <iostream>
#include "videostate.h"

using namespace player;

VideoState::~VideoState()
{
  if (m_flushPkt)
  {
    // wipe the packet
    av_packet_unref(m_flushPkt);
  }
}

void VideoState::allocPicture()
{
  m_flushPkt = av_packet_alloc();
  m_flushPkt->data = (uint8_t*)"FLUSH";

  auto videoPicture = &pictq[m_pictqWindex];

  // check if the sdl_overlay is allocated
  if (videoPicture->frame)
  {
    // we already have an avframe allocated, free memory
    av_frame_free(&videoPicture->frame);
    av_free(videoPicture->frame);
  }

  // lock global screen mutex
  SDL_LockMutex(m_screenMutex);

  // get the size in bytes required to store an image with the given parameters
  int numBytes = 0;
  numBytes = av_image_get_buffer_size(
    AV_PIX_FMT_YUV420P
    , m_videoCtx->width
    , m_videoCtx->height
    , 32
    );

  // allocate image data buffer
  auto buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

  // alloc the avframe later used to contain the scaled frame
  videoPicture->frame = av_frame_alloc();
  if (videoPicture->frame == nullptr)
  {
    return;
  }

  // the fields of the given image are filled in by using the buffer which points to the image data buffer
  av_image_fill_arrays(
    videoPicture->frame->data
    , videoPicture->frame->linesize
    , buffer
    , AV_PIX_FMT_YUV420P
    , m_videoCtx->width
    , m_videoCtx->height
    , 32
    );

  // unlock mutex
  SDL_UnlockMutex(m_screenMutex);

  // update videoPicture struct fields
  videoPicture->width = m_videoCtx->width;
  videoPicture->height = m_videoCtx->height;
  videoPicture->allocated = 1;
}

int VideoState::queuePicture(AVFrame* pFrame, const double& pts)
{
  // lock videostate pictq mutex
  SDL_LockMutex(pictq_mutex);

  // wait until we have space for a new picture in pictq
  while (m_pictqSize >= VIDEO_PICTURE_QUEUE_SIZE)
  {
    SDL_CondWait(pictq_cond, pictq_mutex);
  }

  // unlock pictq mutex
  SDL_UnlockMutex(pictq_mutex);

  auto videoPicture = &pictq[m_pictqWindex];

  // if the videopicture sdl_overlay is not allocated or has a different width, height
  if (!videoPicture->frame ||
      videoPicture->width != m_videoCtx->width ||
      videoPicture->height != m_videoCtx->height)
  {
    // set sdl_overlay not allocated
    videoPicture->allocated = 0;

    // allocate a new sdl_overlay for the videoPicture struct
    this->allocPicture();
  }

  // check the new sdl_overlay was correctly allocated
  if (videoPicture->frame)
  {
    // so now we've got pictures lining up onto our picture queue with proper PTS values
    videoPicture->pts = pts;

    // set videopicture avframe info using the last decoded frame
    videoPicture->frame->pict_type = pFrame->pict_type;
    videoPicture->frame->pts = pFrame->pts;
    videoPicture->frame->pkt_dts = pFrame->pkt_dts;
    videoPicture->frame->key_frame = pFrame->key_frame;
    videoPicture->frame->best_effort_timestamp = pFrame->best_effort_timestamp;
    videoPicture->frame->width = pFrame->width;
    videoPicture->frame->height = pFrame->height;

    // scale the image in pFrame->data and put the resulting scaled image in pict->data
    sws_scale(
      m_decodeVideoSwsCtx
      , (uint8_t const* const*)pFrame->data
      , pFrame->linesize
      , 0
      , m_videoCtx->height
      , videoPicture->frame->data
      , videoPicture->frame->linesize
      );

    // update videopicture queue write index
    m_pictqWindex++;

    // if the write index has reached the videopicture queue size
    if (m_pictqWindex == VIDEO_PICTURE_QUEUE_SIZE)
    {
      m_pictqWindex = 0;
    }

    // lock videopicture queue
    SDL_LockMutex(pictq_mutex);

    // increase videopictq queue size
    m_pictqSize++;

    // unlock videopicture queue
    SDL_UnlockMutex(pictq_mutex);
  }

  return 0;
}

double VideoState::masterClock()
{
  switch (m_avSyncType)
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

  std::cerr << "Error : Undefined a/v sync type" << std::endl;
  return -1;
}

double VideoState::calcVideoClock()
{
  double delta = (av_gettime() - m_videoDecodeCurrentPtsTime) / 1000000.0;
  return m_videoDecodeCurrentPts + delta;
}

double VideoState::calcAudioClock()
{
  double pts = m_audioClock;
  int hw_buf_size = m_audioBufSize - m_audioBufIndex;
  int bytes_per_sec = 0;
  int n = 2 * m_audioCtx->ch_layout.nb_channels;

  if (m_audioStream)
  {
    bytes_per_sec = m_audioCtx->sample_rate * n;
  }

  if (bytes_per_sec)
  {
    pts -= (double) hw_buf_size / bytes_per_sec;
  }

  return pts;
}

double VideoState::calcExternalClock()
{
  m_externalClockTime = av_gettime();
  m_externalClock = m_externalClockTime / 1000000.0;

  return m_externalClock;
}


void VideoState::streamSeek(const int64_t& pos, const int& rel)
{
  if (!m_seekReq)
  {
    m_seekPos = pos;
    m_seekFlags = rel < 0 ? AVSEEK_FLAG_BACKWARD : 0;
    m_seekReq = 1;
  }
}
