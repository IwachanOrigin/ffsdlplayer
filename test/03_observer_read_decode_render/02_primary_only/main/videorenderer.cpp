
#if (WIN32)
#include <windows.h>
#endif
#include <iostream>
#include <thread>
#include <chrono>
#include "videorenderer.h"

#define FF_REFRESH_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT    (SDL_USEREVENT + 1)

// av sync correction is done if the clock difference is above the max av sync threshold
constexpr double AV_SYNC_THRESHOLD = 0.01;

// no av sync correction is done if the clock difference is below the minimum av sync shreshold
constexpr double AV_NOSYNC_THRESHOLD = 1.0;

using namespace player;

VideoRenderer::VideoRenderer()
  : Subject()
{
  m_mutex = SDL_CreateMutex();
}

VideoRenderer::~VideoRenderer()
{
  this->stop();

  if (m_mutex)
  {
    SDL_DestroyMutex(m_mutex);
    m_mutex = nullptr;
  }

  if (m_texture)
  {
    SDL_DestroyTexture(m_texture);
    m_texture = nullptr;
  }

  if (m_renderer)
  {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }

  if (m_screen)
  {
    SDL_DestroyWindow(m_screen);
    m_screen = nullptr;
  }

}

int VideoRenderer::start(std::shared_ptr<GlobalState> gs)
{
  m_gs = gs;
  if (!m_gs)
  {
    return -1;
  }

  std::thread([&](VideoRenderer *vr)
  {
    vr->displayThread();
  }, this).detach();
  return 0;
}

void VideoRenderer::stop()
{
  m_isRendererFinished = true;
  std::chrono::milliseconds ms(100);
  std::this_thread::sleep_for(ms);
}

int VideoRenderer::displayThread()
{
  SDL_Event event;
  int ret = -1;

  this->scheduleRefresh(100);

  while (1)
  {
    if (m_isRendererFinished)
    {
      break;
    }

    double incr = 0, pos = 0;
    // wait indefinitely for the next available event
    ret = SDL_WaitEvent(&event);
    if (ret == 0)
    {
      std::cerr << "SDL_WaitEvent failed : " << SDL_GetError() << std::endl;
    }

    // switch on the retrieved event type
    switch (event.type)
    {
      case SDL_KEYDOWN:
      {
        switch (event.key.keysym.sym)
        {
          case SDLK_LEFT:
          {
            incr = -10.0;
            goto do_seek;
          }
          break;

          case SDLK_RIGHT:
          {
            incr = 10.0;
            goto do_seek;
          }
          break;

          case SDLK_DOWN:
          {
            incr = -60.0;
            goto do_seek;
          }
          break;

          case SDLK_UP:
          {
            incr = 60.0;
            goto do_seek;
          }
          break;

          do_seek:
          {
            if (m_gs)
            {
              pos = m_gs->masterClock();
              pos += incr;
              m_gs->streamSeek((int64_t)(pos * AV_TIME_BASE), incr);
            }
            break;
          }

          default:
          {
            // nothing
          }
          break;
        }
      }
      break;

      case FF_QUIT_EVENT:
      case SDL_QUIT:
      {
        m_isRendererFinished = true;
      }
      break;

      case FF_REFRESH_EVENT:
      {
        this->videoRefreshTimer();
      }
      break;
    }
  }

  if (m_texture)
  {
    SDL_DestroyTexture(m_texture);
    m_texture = nullptr;
  }

  if (m_renderer)
  {
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;
  }

  if (m_screen)
  {
    SDL_DestroyWindow(m_screen);
    m_screen = nullptr;
  }

  return 0;
}

void VideoRenderer::scheduleRefresh(int delay)
{
  // schedule an sdl timer
  int ret = SDL_AddTimer(delay, this->sdlRefreshTimerCb, m_gs.get());
  if (ret == 0)
  {
    std::cerr << "could not schedule refresh callback : " << SDL_GetError() << std::endl;
  }
}

void VideoRenderer::videoRefreshTimer()
{
  // Check the video stream was correctly opened
  auto& videoStream = m_gs->videoStream();
  if (!videoStream)
  {
    this->scheduleRefresh(100);
    return;
  }

  // Check the videopicture queue contains decoded frames
  if (m_gs->sizeVideoFrameDecoded() == 0)
  {
    if (m_isRendererFinished)
    {
      this->notifyObservers();
      return;
    }
    this->scheduleRefresh(1);
    return;
  }

  // Get videopicture reference using the queue read index
  AVFrame* frame = av_frame_alloc();
  if (!frame)
  {
    std::cerr << "Could not allocate AVFrame" << std::endl;
    return;
  }
  auto isFrame = m_gs->popVideoFrameDecoded(frame);
  if (isFrame < 0)
  {
    std::cerr << "Could not get AVFrame" << std::endl;
    // wipe the frame
    av_frame_free(&frame);
    av_free(frame);
    return;
  }

  // Get last frame pts
  auto frameDecodeLastPts = m_gs->frameDecodeLastPts();
  auto ptsDelay = frame->pts - frameDecodeLastPts;

  // If the obtained delay is incorrect
  if (ptsDelay <= 0 || ptsDelay >= 1.0)
  {
    // Use the previously calculated delay
    ptsDelay = frameDecodeLastPts;
  }

  // Save delay information for the next time
  m_gs->setFrameDecodeLastDelay(ptsDelay);
  m_gs->setFrameDecodeLastPts(frame->pts);

  // Update delay to stay in sync with the audio
  auto audioRefClock = m_gs->calcAudioClock();
  auto audioVideoDelay = frame->pts - audioRefClock;

  // Skip or repeat the frame taking into account the delay
  auto syncThreshold = (ptsDelay > AV_SYNC_THRESHOLD) ? ptsDelay : AV_SYNC_THRESHOLD;

  // Check audio video delay absolute value is below sync threshold
  if (fabs(audioVideoDelay) < AV_NOSYNC_THRESHOLD)
  {
    if (audioVideoDelay <= -syncThreshold)
    {
      ptsDelay = 0;
    }
    else if (audioVideoDelay >= syncThreshold)
    {
      ptsDelay = 2 * ptsDelay;
    }
  }

  auto frameDecodeTimer = m_gs->frameDecodeTimer() + ptsDelay;
  m_gs->setFrameDecodeTimer(frameDecodeTimer);
  // Compute the real delay
  auto realDelay = frameDecodeTimer - (av_gettime() / 1000000.0);
  if (realDelay < 0.010)
  {
    realDelay = 0.010;
  }

  this->scheduleRefresh((int)(realDelay * 1000 + 0.5));

  // Show the frame on the sdl_surface
  this->videoDisplay(frame);

  // wipe the frame
  av_frame_free(&frame);
  av_free(frame);
}

Uint32 VideoRenderer::sdlRefreshTimerCb(Uint32 interval, void* param)
{
  // Create an sdl event of type
  SDL_Event event;
  event.type = FF_REFRESH_EVENT;
  event.user.data1 = param;

  // Push the event to the events queue
  SDL_PushEvent(&event);

  return 0;
}

void VideoRenderer::videoDisplay(AVFrame* frame)
{
  auto& videoCodecCtx = m_gs->videoCodecCtx();
  // Create window, renderer and textures if not already created
  if (!m_screen)
  {
    //int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS | SDL_WINDOW_TOOLTIP;
    int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE;
    m_screen = SDL_CreateWindow(
      "display"
      , SDL_WINDOWPOS_UNDEFINED
      , SDL_WINDOWPOS_UNDEFINED
      , videoCodecCtx->width / 2
      , videoCodecCtx->height / 2
      , flags
      );
    SDL_GL_SetSwapInterval(1);
  }

  // Check window was correctly created
  if (!m_screen)
  {
    std::cerr << "SDL : could not create window - exiting" << std::endl;
    return;
  }

  if (!m_renderer)
  {
    // Create a 2d rendering context for the sdl_window
    m_renderer = SDL_CreateRenderer(
      m_screen
      , -1
      , SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE
      );
  }

  if (!m_texture)
  {
    // Create a texture for a rendering context
    m_texture = SDL_CreateTexture(
      m_renderer
      , SDL_PIXELFORMAT_YV12
      , SDL_TEXTUREACCESS_STREAMING
      , videoCodecCtx->width
      , videoCodecCtx->height
      );
  }
  // Reference for the next videopicture to be displayed
  float aspectRatio = 0;
  int w = 0, h = 0, x = 0, y = 0;

  // Get next videoPicture to be displayed from the videopicture queue
  // Get videopicture reference using the queue read index
  if (frame)
  {
    if (videoCodecCtx->sample_aspect_ratio.num == 0)
    {
      aspectRatio = 0;
    }
    else
    {
      aspectRatio = av_q2d(videoCodecCtx->sample_aspect_ratio) * videoCodecCtx->width / videoCodecCtx->height;
    }

    if (aspectRatio <= 0.0)
    {
      aspectRatio = (float)videoCodecCtx->width / (float)videoCodecCtx->height;
    }

    // Get the size of a window's client area
    int screenWidth = 0;
    int screenHeight = 0;
    SDL_GetWindowSize(m_screen, &screenWidth, &screenHeight);

    // Global sdl_surface height
    h = screenHeight;

    // Retrieve width using the calculated aspect ratio and the screen height
    w = ((int) rint(h * aspectRatio)) & -3;

    // If the new width is bigger than the screen width
    if (w > screenWidth)
    {
      // Set the width to the screen width
      w = screenWidth;
      // Recalculate height using the calculated aspect ratio and screen width
      h = ((int) rint(w / aspectRatio)) & -3;
    }

    x = (screenWidth - w);
    y = (screenHeight - h);

    // Check the number of frames to decode was not exceeded
    {
      // Set blit area x and y coordinates, width and height
      SDL_Rect rect{};
      rect.x = x;
      rect.y = y;
      rect.w = 2 * w;
      rect.h = 2 * h;

      // Lock screen mutex
      SDL_LockMutex(m_mutex);

      // Update the texture with the new pixel data
      SDL_UpdateYUVTexture(
        m_texture
        , &rect
        , frame->data[0]
        , frame->linesize[0]
        , frame->data[1]
        , frame->linesize[1]
        , frame->data[2]
        , frame->linesize[2]
        );

      // Clear the current rendering target with the drawing color
      SDL_RenderClear(m_renderer);

      // Copy a portion of the texture to the current rendering target
      SDL_RenderCopy(m_renderer, m_texture, &rect, nullptr);

      // Update the screen with any rendering performed since the previous call
      SDL_RenderPresent(m_renderer);

      // Unlock screen mutex
      SDL_UnlockMutex(m_mutex);
    }
  }
}

