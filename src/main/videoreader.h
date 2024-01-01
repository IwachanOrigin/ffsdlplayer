
#ifndef VIDEO_READER_H_
#define VIDEO_READER_H_

#include <string>
#include <memory>

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

namespace player
{

class VideoState;
class VideoDecoder;
class VideoRenderer;

class VideoReader
{
public:
  explicit VideoReader() = default;
  ~VideoReader() = default;

  int start(const std::string& filename, const int& audioDeviceIndex);
  void stop();

private:
  std::shared_ptr<VideoState> m_videoState;
  std::unique_ptr<VideoDecoder> m_videoDecoder;
  std::unique_ptr<VideoRenderer> m_videoRenderer;
  std::string m_filename = "";

  int streamComponentOpen(std::shared_ptr<VideoState> vs, const int& streamIndex);
  int releaseAll();
  int readThread(std::shared_ptr<VideoState> vs);
};

}

#endif // VIDEO_READER_H_
