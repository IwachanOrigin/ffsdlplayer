
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

  void start(std::vector<std::string_view>& filenames);
  bool isFinished() const { return m_isFinished; }

  // From Observer class
  virtual void update(Subject* subject);

private:
  std::shared_ptr<GlobalState> m_primaryGlobalState = nullptr;
  std::shared_ptr<GlobalState> m_secondaryGlobalState = nullptr;
  std::unique_ptr<VideoReader> m_videoReader = nullptr;
  std::unique_ptr<VideoDecoder> m_videoDecoder = nullptr;

  std::atomic_bool m_isFinished = false;
  std::atomic_bool m_isVideoReaderFinished = false;
  std::atomic_bool m_isVideoDecoderFinished = false;

  std::vector<std::string_view> m_movFileVec;
};

} // player

#endif // VIDEO_CONTROLLER_H_

