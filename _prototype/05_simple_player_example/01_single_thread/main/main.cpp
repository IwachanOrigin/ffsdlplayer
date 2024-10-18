
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstdbool>
#include <ctime>


extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <SDL.h>
}


void display(AVCodecContext*, AVPacket*, AVFrame*, SDL_Rect*, SDL_Texture*, SDL_Renderer*, double);
void playaudio(AVCodecContext*, AVPacket*, AVFrame*, SDL_AudioDeviceID);

// グローバル変数としてウィンドウとレンダラーを保持
SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_AudioDeviceID auddev;

void play_video(const char* filename)
{
  AVFormatContext *pFormatCtx = nullptr;
  int vidId = -1, audId = -1;
  double fpsrendering = 0.0;
  AVCodecContext *vidCtx, *audCtx;
  AVCodecParameters *vidpar, *audpar;
  AVFrame *vframe, *aframe;
  AVPacket *packet;
  SDL_Rect rect;
  SDL_AudioSpec want, have;
  bool running = true;

  pFormatCtx = avformat_alloc_context();
  char bufmsg[1024]{};
  if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) < 0)
  {
    sprintf(bufmsg, "Cannot open %s", filename);
    perror(bufmsg);
    //goto clean_format_context;
    exit(EXIT_FAILURE);
  }
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
  {
    perror("Cannot find stream info. Quitting.");
    //goto clean_format_context;
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < pFormatCtx->nb_streams; i++)
  {
    AVCodecParameters *localparam = pFormatCtx->streams[i]->codecpar;
    if (localparam->codec_type == AVMEDIA_TYPE_VIDEO)
    {
      const AVCodec* vidCodec = avcodec_find_decoder(localparam->codec_id);
      vidpar = localparam;
      vidId = i;
      AVRational rational = pFormatCtx->streams[i]->avg_frame_rate;
      fpsrendering = 1.0 / ((double)rational.num / (double)(rational.den));
      vidCtx = avcodec_alloc_context3(vidCodec);
      if (avcodec_parameters_to_context(vidCtx, vidpar) < 0)
      {
        perror("vidCtx");
        //goto clean_codec_context;
        exit(EXIT_FAILURE);
      }

      if (avcodec_open2(vidCtx, vidCodec, NULL) < 0)
      {
        perror("vidCtx");
        //goto clean_codec_context;
        exit(EXIT_FAILURE);
      }
    }
    else if (localparam->codec_type == AVMEDIA_TYPE_AUDIO)
    {
      const AVCodec* audCodec = avcodec_find_decoder(localparam->codec_id);
      audpar = localparam;
      audId = i;
      audCtx = avcodec_alloc_context3(audCodec);
      if (avcodec_parameters_to_context(audCtx, audpar) < 0)
      {
        perror("audCtx");
        //goto clean_codec_context;
        exit(EXIT_FAILURE);
      }

      if (avcodec_open2(audCtx, audCodec, NULL) < 0)
      {
        perror("audCtx");
        //goto clean_codec_context;
        exit(EXIT_FAILURE);
      }
    }
  }

  vframe = av_frame_alloc();
  aframe = av_frame_alloc();
  packet = av_packet_alloc();

  // ウィンドウとレンダラーを初期化（最初の一回だけ）
  if (screen == NULL)
  {
    screen = SDL_CreateWindow("Fplay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              vidpar->width, vidpar->height, SDL_WINDOW_OPENGL);
    if (!screen)
    {
      perror("screen");
      //goto clean_renderer;
      exit(EXIT_FAILURE);
    }
    renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
      perror("renderer");
      //goto clean_renderer;
      exit(EXIT_FAILURE);
    }
  }

  // テクスチャの更新
  if (texture != NULL) {
    SDL_DestroyTexture(texture);
  }
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                              SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET,
                              vidpar->width, vidpar->height);
  if (!texture)
  {
    perror("texture");
    //goto clean_texture;
    exit(EXIT_FAILURE);
  }

  rect.x = 0;
  rect.y = 0;
  rect.w = vidpar->width;
  rect.h = vidpar->height;

  SDL_zero(want);
  SDL_zero(have);
  want.samples = audpar->sample_rate;
  want.channels = audpar->ch_layout.nb_channels;
  auddev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
  SDL_PauseAudioDevice(auddev, 0);
  if (!auddev)
  {
    perror("auddev");
    //goto clean_audio_device;
    exit(EXIT_FAILURE);
  }
  SDL_Event evt;
  uint32_t windowID = SDL_GetWindowID(screen);
  
  while (running)
  {
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
      while (SDL_PollEvent(&evt))
      {
        if (evt.type == SDL_QUIT)
        {
          running = false;
          break;
        }
      }
      if (packet->stream_index == vidId)
      {
        display(vidCtx, packet, vframe, &rect, texture, renderer, fpsrendering);
      } else if (packet->stream_index == audId) {
        //playaudio(audCtx, packet, aframe, auddev);
      }
      av_packet_unref(packet);
    }
    break;
  }

clean_audio_device:
  SDL_CloseAudioDevice(auddev);
clean_texture:
  SDL_DestroyTexture(texture);
clean_renderer:
  av_packet_free(&packet);
  av_frame_free(&vframe);
  av_frame_free(&aframe);
clean_codec_context:
  avcodec_free_context(&vidCtx);
  avcodec_free_context(&audCtx);
clean_format_context:
  avformat_close_input(&pFormatCtx);
  avformat_free_context(pFormatCtx);
}

void display(AVCodecContext* vidCtx, AVPacket* packet, AVFrame* frame, SDL_Rect* rect, 
             SDL_Texture* texture, SDL_Renderer* renderer, double fpsrendering)
{
  // パケットをデコードしてフレームに変換
  if (avcodec_send_packet(vidCtx, packet) == 0)
  {
    while (avcodec_receive_frame(vidCtx, frame) == 0)
    {
      // フレームをSDLテクスチャにコピー
      SDL_UpdateYUVTexture(texture, rect, 
                           frame->data[0], frame->linesize[0],
                           frame->data[1], frame->linesize[1],
                           frame->data[2], frame->linesize[2]);
      // 画面にレンダリング
      SDL_RenderClear(renderer);
      SDL_RenderCopy(renderer, texture, NULL, rect);
      SDL_RenderPresent(renderer);

      // フレームレートに応じた待機
      SDL_Delay(static_cast<Uint32>(fpsrendering * 1000));
    }
  }
}

void playaudio(AVCodecContext* audCtx, AVPacket* packet, AVFrame* frame, SDL_AudioDeviceID auddev)
{
  // パケットをデコードしてオーディオフレームに変換
  if (avcodec_send_packet(audCtx, packet) == 0)
  {
    while (avcodec_receive_frame(audCtx, frame) == 0)
    {
      // オーディオデータをSDLに送る
      SDL_QueueAudio(auddev, frame->data[0], frame->linesize[0]);
    }
  }
}


#undef main

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("usage: %s <filename1> <filename2> ...\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  for (int i = 1; i < argc; ++i)
  {
    play_video(argv[i]);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(screen);
  SDL_Quit();

  return 0;
}


