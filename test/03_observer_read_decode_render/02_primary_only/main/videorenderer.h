
#ifndef VIDEO_RENDERER_H_
#define VIDEO_RENDERER_H_

extern "C"
{
#include <SDL.h>
}

#include "subject.h"
#include "globalstate.h"
#include <atomic>

namespace player
{

class VideoRenderer : public Subject
{
public:
  explicit VideoRenderer();
  virtual ~VideoRenderer();

  int start(std::shared_ptr<GlobalState> gs);
  void stop();

private:
  std::shared_ptr<GlobalState> m_gs = nullptr;
  SDL_Window* m_screen = nullptr;
  SDL_Texture* m_texture = nullptr;
  SDL_Renderer* m_renderer = nullptr;
  SDL_mutex* m_mutex = nullptr;
  std::atomic_bool m_isRendererFinished = false;

  int displayThread();
  void scheduleRefresh(int delay);
  void videoRefreshTimer();
  static Uint32 sdlRefreshTimerCb(Uint32 interval, void* param);
  void videoDisplay(AVFrame* frame);
};

} // player

#endif // VIDEO_RENDERER_H_

