
#ifndef VIDEO_DECODER_H_
#define VIDEO_DECODER_H_

extern "C"
{
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "subject.h"
#include "globalstate.h"
#include "videorenderer.h"

namespace player
{

class VideoDecoder : public Subject
{
public:
  explicit VideoDecoder();
  virtual ~VideoDecoder();

  int start(std::shared_ptr<GlobalState> vs);
  void stop();

private:
  std::shared_ptr<GlobalState> m_gs = nullptr;
  std::unique_ptr<VideoRenderer> m_videoRenderer = nullptr;
  std::mutex m_mutex;
  bool m_finishedDecoder = false;

  int decodeThread(std::shared_ptr<GlobalState> vs);
  int64_t guessCorrectPts(AVCodecContext* ctx, const int64_t& reordered_pts, const int64_t& dts);
  double syncVideo(std::shared_ptr<GlobalState> vs, AVFrame* srcFrame, double pts);
};

} // player

#endif // VIDEO_DECODER_H_
