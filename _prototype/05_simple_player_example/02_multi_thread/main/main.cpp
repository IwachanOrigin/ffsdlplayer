#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstdbool>
#include <ctime>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include <SDL.h>
}

#undef main

void displayFrame(AVFrame*, SDL_Rect*, SDL_Texture*, SDL_Renderer*, double);
void playaudio(AVCodecContext*, AVPacket*, AVFrame*, SDL_AudioDeviceID);

// �O���[�o���ϐ��Ƃ��ăE�B���h�E�ƃ����_���[��ێ�
SDL_Window *screen = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *texture = NULL;
SDL_AudioDeviceID auddev;

// �X���b�h�Ԃ̃f�[�^���L�̂��߂̃L���[�Ɠ����p�̕ϐ�
std::queue<AVPacket*> packetQueue;
std::queue<AVFrame*> frameQueue;
std::mutex packetMutex, frameMutex;
std::condition_variable packetCv, frameCv, renderFinishedCv;
bool readFinished = false;
bool decodeFinished = false;
bool renderFinished = false;

void readThread(AVFormatContext* pFormatCtx)
{
    AVPacket* packet = av_packet_alloc();
    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        {
            std::unique_lock<std::mutex> lock(packetMutex);
            packetQueue.push(packet);
        }
        packetCv.notify_one();  // �f�R�[�h�X���b�h�ɒʒm
        packet = av_packet_alloc();  // �V�����p�P�b�g������
    }
    {
        std::unique_lock<std::mutex> lock(packetMutex);
        readFinished = true;
    }
    packetCv.notify_all();  // �f�R�[�h�X���b�h�ɏI����ʒm
}

void decodeThread(AVCodecContext* vidCtx, AVCodecContext* audCtx, int vidId, int audId)
{
  AVFrame* vframe = av_frame_alloc();
  AVFrame* aframe = av_frame_alloc();
  while (true)
  {
    std::unique_lock<std::mutex> lock(packetMutex);
    if (packetQueue.empty() && !readFinished) {
      std::cout << "decodeThread: waiting on packetCv - packetQueue is empty, read is not finished" << std::endl;
    }
    packetCv.wait(lock, [] { return !packetQueue.empty() || readFinished; });

    if (packetQueue.empty() && readFinished)
    {
      break;  // �ǂݎ�肪�I�����Ă��āA�������ׂ��p�P�b�g���Ȃ��ꍇ�I��
    }

    AVPacket* packet = packetQueue.front();
    packetQueue.pop();
    lock.unlock();

    if (packet->stream_index == vidId)
    {
      if (avcodec_send_packet(vidCtx, packet) == 0)
      {
        while (avcodec_receive_frame(vidCtx, vframe) == 0)
        {
          {
            std::unique_lock<std::mutex> frameLock(frameMutex);
            frameQueue.push(av_frame_clone(vframe));
          }
          frameCv.notify_one();  // �����_�����O�X���b�h�ɒʒm
        }
      }
    }
    else if (packet->stream_index == audId)
    {
      //playaudio(audCtx, packet, aframe, auddev);
    }
    av_packet_unref(packet);
  }
  {
    std::unique_lock<std::mutex> lock(frameMutex);
    decodeFinished = true;
  }
  frameCv.notify_all();  // �����_�����O�X���b�h�ɏI����ʒm
  av_frame_free(&vframe);
  av_frame_free(&aframe);
}

void renderThread(SDL_Rect* rect, double fpsrendering)
{
  SDL_Event event;
  while (true)
  {
    // SDL�C�x���g������
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        // �E�B���h�E�����C�x���g�����������ꍇ�A�I��
        {
          std::unique_lock<std::mutex> lock(frameMutex);
          decodeFinished = true;
          renderFinished = true;
        }
        frameCv.notify_all();
        renderFinishedCv.notify_one();
        return;
      }
    }

    std::unique_lock<std::mutex> lock(frameMutex);
    if (frameQueue.empty() && !decodeFinished) {
      std::cout << "renderThread: waiting - frameQueue is empty, decode is not finished" << std::endl;
      lock.unlock();
      SDL_Delay(10);  // �X���[�v���g�킸�A�Z���ҋ@�Ń��[�v����
      continue;
    }

    if (frameQueue.empty() && decodeFinished)
    {
      break;  // �f�R�[�h���I�����Ă��āA�������ׂ��t���[�����Ȃ��ꍇ�I��
    }

    AVFrame* frame = frameQueue.front();
    frameQueue.pop();
    lock.unlock();

    displayFrame(frame, rect, texture, renderer, fpsrendering);
    av_frame_free(&frame);
  }
  {
    std::unique_lock<std::mutex> lock(frameMutex);
    renderFinished = true;
  }
  renderFinishedCv.notify_one();
}


void play_video(const char* filename)
{
    AVFormatContext *pFormatCtx = nullptr;
    int vidId = -1, audId = -1;
    double fpsrendering = 0.0;
    AVCodecContext *vidCtx, *audCtx;
    AVCodecParameters *vidpar, *audpar;
    SDL_Rect rect;
    SDL_AudioSpec want, have;

    pFormatCtx = avformat_alloc_context();
    char bufmsg[1024]{};
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) < 0)
    {
        sprintf(bufmsg, "Cannot open %s", filename);
        perror(bufmsg);
        exit(EXIT_FAILURE);
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        perror("Cannot find stream info. Quitting.");
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
                exit(EXIT_FAILURE);
            }

            if (avcodec_open2(vidCtx, vidCodec, NULL) < 0)
            {
                perror("vidCtx");
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
                exit(EXIT_FAILURE);
            }

            if (avcodec_open2(audCtx, audCodec, NULL) < 0)
            {
                perror("audCtx");
                exit(EXIT_FAILURE);
            }
        }
    }

    // �E�B���h�E�ƃ����_���[���������i�ŏ��̈�񂾂��j
    if (screen == NULL)
    {
        screen = SDL_CreateWindow("Fplay", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  vidpar->width, vidpar->height, SDL_WINDOW_OPENGL);
        if (!screen)
        {
            perror("screen");
            exit(EXIT_FAILURE);
        }
        renderer = SDL_CreateRenderer(screen, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer)
        {
            perror("renderer");
            exit(EXIT_FAILURE);
        }
    }

    // �e�N�X�`���̍X�V
    if (texture != NULL) {
        SDL_DestroyTexture(texture);
    }
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV,
                                SDL_TEXTUREACCESS_STREAMING | SDL_TEXTUREACCESS_TARGET,
                                vidpar->width, vidpar->height);
    if (!texture) {
        perror("texture");
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
        exit(EXIT_FAILURE);
    }

    // �X���b�h�̊J�n
    std::thread reader(readThread, pFormatCtx);
    std::thread decoder(decodeThread, vidCtx, audCtx, vidId, audId);
    std::thread rendererThread(renderThread, &rect, fpsrendering);

    reader.detach();
    decoder.detach();
    rendererThread.detach();

    // �S�ẴX���b�h���I������̂�ҋ@
    std::unique_lock<std::mutex> lock(frameMutex);
    std::cout << "mainThread: waiting on renderFinishedCv" << std::endl;
    renderFinishedCv.wait(lock, [] { return renderFinished; });

    // �㏈��
    SDL_CloseAudioDevice(auddev);
    avcodec_free_context(&vidCtx);
    avcodec_free_context(&audCtx);
    avformat_close_input(&pFormatCtx);
    avformat_free_context(pFormatCtx);
}

void displayFrame(AVFrame* frame, SDL_Rect* rect, SDL_Texture* texture, SDL_Renderer* renderer, double fpsrendering)
{
  SDL_UpdateYUVTexture(texture, rect,
                       frame->data[0], frame->linesize[0],
                       frame->data[1], frame->linesize[1],
                       frame->data[2], frame->linesize[2]);
  //SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // �F
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, NULL, rect);
  SDL_RenderPresent(renderer);
  SDL_Delay(static_cast<Uint32>(fpsrendering * 1000));
}

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
        // �e�X���b�h�̊����t���O�����Z�b�g
        readFinished = false;
        decodeFinished = false;
        renderFinished = false;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
    SDL_Quit();

    return 0;
}
