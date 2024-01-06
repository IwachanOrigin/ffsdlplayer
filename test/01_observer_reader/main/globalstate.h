
#ifndef GLOBAL_STATE_H_
#define GLOBAL_STATE_H_

extern "C"
{
#include "libavcodec/avcodec.h"
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
  AVCodecContext* videoCodecCtx = nullptr;
  AVStream* videoStream = nullptr;
  int videoStreamIndex = -1;

  // For Audio
  AVCodecContext* audioCodecCtx = nullptr;
  AVStream* audioStream = nullptr;
  int audioStreamIndex = -1;

  // For reader
  PacketQueue audioPacketQueueRead;
  PacketQueue videoPacketQueueRead;
};

class GlobalState
{
public:
  explicit GlobalState();
  ~GlobalState();

  // Init
  int setup(const std::string& filename);

  // Common
  AVFormatContext*& inputFmtCtx() const { return m_vs->inputFmtCtx; }
  int& videoStreamIndex() const { return m_vs->videoStreamIndex; }
  int& audioStreamIndex() const { return m_vs->audioStreamIndex; }
  AVPacket*& flushPacket() const { return m_vs->flushPkt; }
  AVCodecContext*& videoCodecCtx() const { return m_vs->videoCodecCtx; }
  AVStream*& videoStream() const { return m_vs->videoStream; }
  AVCodecContext*& audioCodecCtx() const { return m_vs->audioCodecCtx; }
  AVStream*& audioStream() const { return m_vs->audioStream; }

  // For Read
  int pushAudioPacketRead(AVPacket* packet);
  int pushVideoPacketRead(AVPacket* packet);
  int popAudioPacketRead(AVPacket* packet);
  int popVideoPacketRead(AVPacket* packet);
  int sizeAudioPacketRead() const { return m_vs->audioPacketQueueRead.size(); }
  int sizeVideoPacketRead() const { return m_vs->videoPacketQueueRead.size(); }
  int nbPacketsAudioRead() const { return m_vs->audioPacketQueueRead.nbPackets(); }
  int nbPacketsVideoRead() const { return m_vs->videoPacketQueueRead.nbPackets(); }
  void clearAudioPacketRead() const { m_vs->audioPacketQueueRead.clear(); }
  void clearVideoPacketRead() const { m_vs->videoPacketQueueRead.clear(); }

private:
  std::unique_ptr<VideoState> m_vs;
  SYNC_TYPE m_clockSyncType;

  int setupComponent(const int& streamIndex);
};

} // player

#endif // GLOBAL_STATE_H_

