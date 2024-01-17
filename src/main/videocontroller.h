
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

  void start(std::vector<std::string>& filenames);
  bool isFinished() const { return m_isFinished; }

  // From Observer class
  virtual void update(Subject* subject);

private:
  std::vector<std::shared_ptr<GlobalState>> m_vecGlobalState;
  std::unique_ptr<VideoReader> m_primaryVideoReader = nullptr;
  std::unique_ptr<VideoReader> m_secondaryVideoReader = nullptr;

  std::unique_ptr<VideoDecoder> m_primaryVideoDecoder = nullptr;
  std::unique_ptr<VideoDecoder> m_secondaryVideoDecoder = nullptr;

  std::atomic_bool m_isFinished = false;

  std::vector<std::string> m_movFileVec;
  int m_startedReadFileCount = 0;
  int m_startedDecodeFileCount = 0;
};

} // player

#endif // VIDEO_CONTROLLER_H_

