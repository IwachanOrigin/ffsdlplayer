
#ifndef VIDEO_STATE_H_
#define VIDEO_STATE_H_

#include <string>
#include "packetqueue.h"
#include "videopicture.h"

extern "C"
{
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

#define VIDEO_PICTURE_QUEUE_SIZE 1

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

class VideoState
{
public:
  explicit VideoState() = default;
  ~VideoState();

  // Common
  AVFormatContext*& formatCtx() { return m_formatCtx; }
  int& videoStreamIndex() { return m_videoStreamIndex; }
  int& audioStreamIndex() { return m_audioStreamIndex; }
  AVPacket*& flushPacket() { return m_flushPkt; }
  AVCodecContext*& videoCodecCtx() { return m_videoCtx; }
  AVStream*& videoStream() { return m_videoStream; }
  AVCodecContext*& audioCodecCtx() { return m_audioCtx; }
  AVStream*& audioStream() { return m_audioStream; }
  int& outputAudioDeviceIndex() { return m_outputAudioDeviceIndex; }
  void setOutputAudioDeviceIndex(const int& outputAudioDeviceIndex) { m_outputAudioDeviceIndex = outputAudioDeviceIndex; }
  int& sdlAudioDeviceID() { return m_sdlAudioDeviceID; }
  void setSdlAudioDeviceID(const int& audioDeviceID) { m_sdlAudioDeviceID = audioDeviceID; };
  int queuePicture(AVFrame* pFrame, const double& pts);

  // For Read
  int pushAudioPacketRead(AVPacket* packet);
  int pushVideoPacketRead(AVPacket* packet);
  int popAudioPacketRead(AVPacket* packet);
  int popVideoPacketRead(AVPacket* packet);
  int sizeAudioPacketRead() const { return m_audioPacketQueue.size(); }
  int sizeVideoPacketRead() const { return m_videoPacketQueue.size(); }
  int nbPacketsAudioRead() const { return m_audioPacketQueue.nbPackets(); }
  int nbPacketsVideoRead() const { return m_videoPacketQueue.nbPackets(); }
  void clearAudioPacketRead() { m_audioPacketQueue.clear(); }
  void clearVideoPacketRead() { m_videoPacketQueue.clear(); }

  // For Decode
  struct SwsContext*& decodeVideoSwsCtx() { return m_decodeVideoSwsCtx; }
  double frameDecodeTimer() const { return m_frameDecodeTimer; }
  void setFrameDecodeTimer(const double& frameTimer) { m_frameDecodeTimer = frameTimer; }
  double frameDecodeLastPts() const { return m_frameDecodeLastPts; }
  void setFrameDecodeLastPts(const double& frameLastPts) { m_frameDecodeLastPts = frameLastPts; }
  double frameDecodeLastDelay() const { return m_frameDecodeLastDelay; }
  void setFrameDecodeLastDelay(const double& frameLastDelay) { m_frameDecodeLastDelay = frameLastDelay; }
  double videoClock() const { return m_videoClock; }
  void setVideoClock(const double& videoClock) { m_videoClock = videoClock; }
  double videoDecodeCurrentPts() const { return m_videoDecodeCurrentPts; }
  void setVideoDecodeCurrentPts(const double& videoCurrentPts) { m_videoDecodeCurrentPts = videoCurrentPts; }
  int64_t videoDecodeCurrentPtsTime() const { return m_videoDecodeCurrentPtsTime; }
  void setVideoDecodeCurrentPtsTime(const int64_t& videoCurrentPtsTime) { m_videoDecodeCurrentPtsTime = videoCurrentPtsTime; }

  // For calculate clock.
  double masterClock();

  // For Seek
  int& seekRequest() { return m_seekReq; }
  void streamSeek(const int64_t& pos, const int& rel);

private:
  void allocPicture();
  double calcVideoClock();
  double calcAudioClock();
  double calcExternalClock();

  AVFormatContext* m_formatCtx = nullptr;

  // audio
  int m_audioStreamIndex = -1;
  AVStream* m_audioStream = nullptr;
  AVCodecContext* m_audioCtx = nullptr;
  PacketQueue m_audioPacketQueue;
  uint8_t m_audioBuf[(MAX_AUDIO_FRAME_SIZE * 3) /2];
  unsigned int m_audioBufSize = 0;
  unsigned int m_audioBufIndex = 0;
  AVFrame m_audioFrame;
  AVPacket m_audioPkt;
  uint8_t* m_audioPktData;
  int m_audioPktSize = 0;
  double m_audioClock = 0.0;

  // video
  int m_videoStreamIndex = -1;
  AVStream* m_videoStream = nullptr;
  AVCodecContext* m_videoCtx = nullptr;
  SDL_Texture* m_texture = nullptr;
  SDL_Renderer* m_renderer = nullptr;
  PacketQueue m_videoPacketQueue;
  struct SwsContext* m_decodeVideoSwsCtx = nullptr;
  double m_frameDecodeTimer = 0.0;
  double m_frameDecodeLastPts = 0.0;
  double m_frameDecodeLastDelay = 0.0;
  double m_videoClock = 0.0;
  double m_videoDecodeCurrentPts = 0.0;
  int64_t m_videoDecodeCurrentPtsTime = 0;
  double m_audioDiffCum = 0.0;
  double m_audioDiffAvgCoef = 0.0;
  double m_audioDiffThreshold = 0.0;
  int m_audioDiffAvgCount = 0;

  // av sync
  SYNC_TYPE m_avSyncType = SYNC_TYPE::AV_SYNC_AUDIO_MASTER;
  double m_externalClock = 0.0;
  int64_t m_externalClockTime = 0;

  // seeking
  int m_seekReq = 0;
  int m_seekFlags = 0;
  int64_t m_seekPos = 0;

  // SDL_surface mutex
  SDL_mutex* m_screenMutex = nullptr;

  // video picture queue
  VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
  int m_pictqSize = 0;
  int m_pictqRindex = 0;
  int m_pictqWindex = 0;
  SDL_mutex* pictq_mutex = nullptr;
  SDL_cond* pictq_cond = nullptr;

  // output audio device index in windows
  int m_outputAudioDeviceIndex = -1;
  int m_sdlAudioDeviceID = -1;

  //
  AVPacket* m_flushPkt = nullptr;

};

} // player

#endif // VIDEO_STATE_H_

