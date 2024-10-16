
#ifndef VIDEO_RENDERER_HPP_
#define VIDEO_RENDERER_HPP_

extern "C"
{
#include <SDL.h>
}

#include "globalstate.hpp"
#include <atomic>
#include <memory>

namespace player
{

class VideoRenderer
{
public:
  explicit VideoRenderer();
  virtual ~VideoRenderer();

  int start(std::shared_ptr<GlobalState> gs);
  void stop();

private:
  std::shared_ptr<GlobalState> m_gs = nullptr;
  std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> m_screen;
  std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> m_renderer;
  std::unique_ptr<SDL_mutex, decltype(&SDL_DestroyMutex)> m_mutex;
  SDL_Texture* m_texture = nullptr;
  std::atomic_bool m_isRendererFinished = false;

  int displayThread();
  void scheduleRefresh(int delay);
  void videoRefreshTimer();
  static Uint32 sdlRefreshTimerCb(Uint32 interval, void* param);
  void videoDisplay(AVFrame* frame);
};

} // player

#endif // VIDEO_RENDERER_HPP_

