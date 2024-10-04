
#ifndef FRAME_QUEUE_HPP_
#define FRAME_QUEUE_HPP_

extern "C"
{
#include "libavutil/frame.h"
}

#include <queue>
#include <mutex>

namespace player
{

struct Frame
{
  AVFrame* frame;
  int width;
  int height;
  double pts;
};

enum class FrameQueueStatus
{
  EMPTY = -1
  , OK    = 0
  , FULL  = 1
};

class FrameQueue
{
public:
  explicit FrameQueue();
  ~FrameQueue();

  int init(const unsigned int& maxSize);
  int pop(AVFrame* frame);
  int push(AVFrame* frame);
  void clear();
  int size() const { return m_qFrames.size(); }

private:
  std::queue<Frame*> m_qFrames;
  std::mutex m_mutex;
  unsigned int m_maxSize;
};

} // player

#endif // FRAME_QUEUE_H_
