
#ifndef VIDEO_CONTROLLER_H_
#define VIDEO_CONTROLLER_H_

#include "observer.h"
#include "globalstate.h"
#include "videoreader.h"

#include <atomic>

namespace player
{

class VideoController : public Observer
{
public:
  explicit VideoController();
  virtual ~VideoController();

  void start(const std::string& filename);
  bool isVideoFinished() const { return m_isVideoFinished; }

  // From Observer class
  virtual void update(Subject* observer);

private:
  std::shared_ptr<GlobalState> m_globalState = nullptr;
  std::unique_ptr<VideoReader> m_primaryVideoReader = nullptr;

  std::atomic_bool m_isVideoFinished = false;
};

} // player

#endif // VIDEO_CONTROLLER_H_
