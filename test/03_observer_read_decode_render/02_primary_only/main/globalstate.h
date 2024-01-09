
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

  // For common the video
  AVCodecContext* videoCodecCtx = nullptr;
  AVStream* videoStream = nullptr;
  int videoStreamIndex = -1;

  // For common the audio
  AVCodecContext* audioCodecCtx = nullptr;
  AVStream* audioStream = nullptr;
  int audioStreamIndex = -1;

  // For reader
  PacketQueue audioPacketQueueRead;
  PacketQueue videoPacketQueueRead;

  // For decoder
  // decode the video
  FrameQueue videoFrameQueueDecoded;
  struct SwsContext* decodeVideoSwsCtx = nullptr;
  double videoClock = 0.0;
  double frameDecodeTimer = 0.0;
  double frameDecodeLastPts = 0.0;
  double frameDecodeLastDelay = 0.0;
  double videoDecodeCurrentPts = 0.0;
  int64_t videoDecodeCurrentPtsTime = 0;

  // decode the audio
  FrameQueue audioFrameQueueDecoded;
  struct SwrContext* decodeAudioSwrCtx = nullptr;
  double audioClock = 0.0;
  unsigned int audioBufSize = 0;
  unsigned int audioBufIndex = 0;
  int audioPktSize = 0;

  // For sync
  double externalClock = 0.0;
  int64_t externalClockTime = 0;
};

class GlobalState
{
public:
  explicit GlobalState();
  ~GlobalState();

  // Init
  int setup(std::string_view filename);

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

  // For Decode
  struct SwsContext*& decodeVideoSwsCtx() const { return m_vs->decodeVideoSwsCtx; }
  struct SwrContext*& decodeAudioSwrCtx() const { return m_vs->decodeAudioSwrCtx; }
  int pushVideoFrameDecoded(AVFrame* frame);
  int pushAudioFrameDecoded(AVFrame* frame);
  int popVideoFrameDecoded(AVFrame* frame);
  int popAudioFrameDecoded(AVFrame* frame);
  int sizeVideoFrameDecoded() const { return m_vs->videoFrameQueueDecoded.size(); }
  int sizeAudioFrameDecoded() const { return m_vs->audioFrameQueueDecoded.size(); }
  void clearAudioFrameDecoded() const { m_vs->audioFrameQueueDecoded.clear(); }
  void clearVideoFrameDecoded() const { m_vs->videoFrameQueueDecoded.clear(); }

  double frameDecodeTimer() const { return m_vs->frameDecodeTimer; }
  void setFrameDecodeTimer(const double& frameTimer) { m_vs->frameDecodeTimer = frameTimer; }
  double frameDecodeLastPts() const { return m_vs->frameDecodeLastPts; }
  void setFrameDecodeLastPts(const double& frameLastPts) { m_vs->frameDecodeLastPts = frameLastPts; }
  double frameDecodeLastDelay() const { return m_vs->frameDecodeLastDelay; }
  void setFrameDecodeLastDelay(const double& frameLastDelay) { m_vs->frameDecodeLastDelay = frameLastDelay; }
  double videoClock() const { return m_vs->videoClock; }
  void setVideoClock(const double& videoClock) { m_vs->videoClock = videoClock; }
  double videoDecodeCurrentPts() const { return m_vs->videoDecodeCurrentPts; }
  void setVideoDecodeCurrentPts(const double& videoCurrentPts) { m_vs->videoDecodeCurrentPts = videoCurrentPts; }
  int64_t videoDecodeCurrentPtsTime() const { return m_vs->videoDecodeCurrentPtsTime; }
  void setVideoDecodeCurrentPtsTime(const int64_t& videoCurrentPtsTime) { m_vs->videoDecodeCurrentPtsTime = videoCurrentPtsTime; }

  // For calculate clock.
  double masterClock();
  double calcAudioClock(); // For AudioDecoder

  // For Seek
  int seekRequest() const { return m_seekReq; }
  void setSeekRequest(const int& req) { m_seekReq = req; }
  int64_t seekPos() const { return m_seekPos; }
  int seekFlags() const { return m_seekFlags; }
  void streamSeek(const int64_t& pos, const int& rel);

private:
  std::unique_ptr<VideoState> m_vs;
  SYNC_TYPE m_clockSyncType;

  // av sync
  SYNC_TYPE m_avSyncType = SYNC_TYPE::AV_SYNC_AUDIO_MASTER;
  double m_externalClock = 0.0;
  int64_t m_externalClockTime = 0;

  // seeking
  int m_seekReq = 0;
  int m_seekFlags = 0;
  int64_t m_seekPos = 0;

  int setupComponent(const int& streamIndex);
  double calcVideoClock();
  double calcExternalClock();
};

} // player

#endif // GLOBAL_STATE_H_

