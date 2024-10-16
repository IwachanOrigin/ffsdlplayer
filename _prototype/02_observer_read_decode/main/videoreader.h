
#ifndef VIDEO_READER_H_
#define VIDEO_READER_H_

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
}

#include "subject.h"

namespace player
{

class GlobalState;

class VideoReader : public Subject
{
public:
  explicit VideoReader();
  virtual ~VideoReader();

  int start(std::shared_ptr<GlobalState> gs);
  void stop();

private:
  std::atomic_bool m_isFinished;

  int readThread(std::shared_ptr<GlobalState> gs);
};

}

#endif // VIDEO_READER_H_
