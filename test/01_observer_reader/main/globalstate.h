
#ifndef GLOBAL_STATE_H_
#define GLOBAL_STATE_H_

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/time.h"
#include "libavdevice/avdevice.h"
}

#include <memory>
#include <mutex>
#include <condition_variable>

#include "framequeue.h"
#include "packetqueue.h"

namespace player
{

enum class SYNC_TYPE
{
  // sync to audio clock
  AV_SYNC_AUDIO_MASTER,
  // sync to video clock
  AV_SYNC_VIDEO_MASTER,
  // sync to external clock : the computer clock
  AV_SYNC_EXTERNAL_MASTER,
};

struct VideoState
{
  // Common
  AVFormatContext* inputFmtCtx = nullptr;
  AVPacket* flushPkt = nullptr;
  bool fileReadEnd = false;
  struct SwsContext* swsCtx = nullptr;

  // For Video
  AVCodecContext* videoCtx = nullptr;
  AVStream* videoStream = nullptr;
  int video_stream_index = -1;

  // For Audio
  AVCodecContext* audioCtx = nullptr;
  AVStream* audioStream = nullptr;
  int audio_stream_index = -1;

  // For reader
  PacketQueue audioPacketQueueRead;
  PacketQueue videoPacketQueueRead;

};

class GlobalState
{
public:
  explicit GlobalState();
  ~GlobalState();

  // Common
  AVFormatContext*& inputFmtCtx() const { return m_videoState->inputFmtCtx; }
  int& videoStreamIndex() const { return m_videoState->video_stream_index; }
  int& audioStreamIndex() const { return m_videoState->audio_stream_index; }
  AVPacket*& flushPacket() const { return m_videoState->flushPkt; }
  AVCodecContext*& videoCodecCtx() const { return m_videoState->videoCtx; }
  AVStream*& videoStream() const { return m_videoState->videoStream; }
  AVCodecContext*& audioCodecCtx() const { return m_videoState->audioCtx; }
  AVStream*& audioStream() const { return m_videoState->audioStream; }

  // For Read
  int pushAudioPacketRead(AVPacket* packet);
  int pushVideoPacketRead(AVPacket* packet);
  int popAudioPacketRead(AVPacket* packet);
  int popVideoPacketRead(AVPacket* packet);
  int sizeAudioPacketRead() const { return m_videoState->audioPacketQueueRead.size(); }
  int sizeVideoPacketRead() const { return m_videoState->videoPacketQueueRead.size(); }
  int nbPacketsAudioRead() const { return m_videoState->audioPacketQueueRead.nbPackets(); }
  int nbPacketsVideoRead() const { return m_videoState->videoPacketQueueRead.nbPackets(); }
  void clearAudioPacketRead() const { m_videoState->audioPacketQueueRead.clear(); }
  void clearVideoPacketRead() const { m_videoState->videoPacketQueueRead.clear(); }

private:
  std::unique_ptr<VideoState> m_videoState;
  SYNC_TYPE m_clockSyncType;

  // Caluculation clock
  double calcVideoClock();
  double calcAudioClock();
  double calcExternalClock();
};

} // player

#endif // GLOBAL_STATE_H_

