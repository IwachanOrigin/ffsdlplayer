
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
  // Primary
  std::shared_ptr<GlobalState> m_primaryGlobalState = nullptr;
  std::unique_ptr<VideoReader> m_primaryVideoReader = nullptr;
  std::unique_ptr<VideoDecoder> m_primaryVideoDecoder = nullptr;

  // Secondary
  std::shared_ptr<GlobalState> m_secondaryGlobalState = nullptr;
  std::unique_ptr<VideoReader> m_secondaryVideoReader = nullptr;
  std::unique_ptr<VideoDecoder> m_secondaryVideoDecoder = nullptr;

  std::atomic_bool m_isFinished = false;
  std::atomic_bool m_isPrimaryVideoReaderStarted = false;
  std::atomic_bool m_isSecondaryVideoReaderStarted = false;
  std::atomic_bool m_isPrimaryVideoReaderFinished = false;
  std::atomic_bool m_isSecondaryVideoReaderFinished = false;
  std::atomic_bool m_isVideoDecoderFinished = false;

  std::vector<std::string_view> m_movFileVec;
};

} // player

#endif // VIDEO_CONTROLLER_H_

