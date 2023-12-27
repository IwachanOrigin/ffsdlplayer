
#ifndef PACKET_QUEUE_H_
#define PACKET_QUEUE_H_

extern "C"
{
#include <SDL.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include "myavpacketlist.h"
#include <queue>

class PacketQueue
{
public:
  explicit PacketQueue();
  ~PacketQueue();

  void init();
  int push(AVPacket* packet);
  int get(AVPacket* packet);
  void clear();
  void flush();

private:
  std::queue<MyAVPacketList*> m_myAvPacketListQueue;
  int m_size;
  int m_nbPackets;
  SDL_cond *cond;
  SDL_mutex *mutex;
};

#endif // PACKET_QUEUE_H_
