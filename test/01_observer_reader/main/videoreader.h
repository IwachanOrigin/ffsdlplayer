
#ifndef VIDEO_READER_H_
#define VIDEO_READER_H_

#include <string>
#include <memory>
#include <atomic>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/avstring.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <SDL.h>
#include <SDL_thread.h>
}

#include "subject.h"

namespace player
{

class GlobalState;

class VideoReader : public Subject
{
public:
  explicit VideoReader() : Subject() {}
  ~VideoReader() = default;

  int start(std::shared_ptr<GlobalState> gs);
  void stop();

private:
  std::shared_ptr<GlobalState> m_gs = nullptr;
  std::atomic_bool m_isFinished = false;

  int readThread(std::shared_ptr<GlobalState> gs);
};

}

#endif // VIDEO_READER_H_
