
#ifndef VIDEO_CONTROLLER_H_
#define VIDEO_CONTROLLER_H_

#include "observer.h"
#include "globalstate.h"
#include "videoreader.h"
#include "videodecoder.h"

#include <atomic>

namespace player
{

class VideoController : public Observer
{
public:
  explicit VideoController();
  virtual ~VideoController();

  void start(std::string_view filename);
  bool isFinished() const { return m_isFinished; }

  // From Observer class
  virtual void update(Subject* subject);

private:
  std::shared_ptr<GlobalState> m_globalState = nullptr;
  std::unique_ptr<VideoReader> m_primaryVideoReader = nullptr;
  std::unique_ptr<VideoDecoder> m_primaryVideoDecoder = nullptr;

  std::atomic_bool m_isFinished = false;
  std::atomic_bool m_isVideoReaderFinished = false;
  std::atomic_bool m_isVideoDecoderFinished = false;
};

} // player

#endif // VIDEO_CONTROLLER_H_

