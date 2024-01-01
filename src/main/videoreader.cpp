
#include <cstring>
#include <thread>

#include "videostate.h"
#include "videoreader.h"

#include "videodecoder.h"
#include "audiodecoder.h"
#include "audioresamplingstate.h"
#include "videorenderer.h"

#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

using namespace player;

int VideoReader::start(const std::string& filename, const int& audioDeviceIndex)
{
  m_videoState = std::make_unique<VideoState>();
  if (m_videoState == nullptr)
  {
    return -1;
  }

  m_filename = filename;
  // set output audio device index
  m_videoState->setOutputAudioDeviceIndex(audioDeviceIndex);

  // start read thread
  std::thread([&](VideoReader *reader)
  {
    reader->readThread(m_videoState);
  }, this).detach();

  return 0;
}

void VideoReader::stop()
{
  
}

int VideoReader::readThread(std::shared_ptr<VideoState> vs)
{
  int ret = -1;

  // retrieve global VideoState reference
  auto videoState = vs;
  AVPacket* packet = nullptr;

  // Set the AVFormatContext for the global videostate ref
  auto formatCtx = videoState->formatCtx();
  AVDictionary* options = nullptr;
  av_dict_set(&options, "rtsp_transport", "tcp", 0);
  ret = avformat_open_input(&formatCtx, m_filename.c_str(), nullptr, &options);
  if (ret < 0)
  {
    std::cerr << "Could not open file " << m_filename << std::endl;
    return -1;
  }
  av_dict_free(&options);
  options = nullptr;

  // reset streamindex
  auto videoStreamIndex = videoState->videoStreamIndex();
  auto audioStreamIndex = videoState->audioStreamIndex();
  videoStreamIndex = -1;
  audioStreamIndex = -1;

  // read packets of the media file to get stream info
  ret = avformat_find_stream_info(formatCtx, nullptr);
  if (ret < 0)
  {
    std::cerr << "Could not find stream info " << m_filename << std::endl;
    return -1;
  }

  // dump info about file onto standard error
  av_dump_format(formatCtx, 0, m_filename.c_str(), 0);

  // loop through the streams that have been found
  for (int i = 0; i < formatCtx->nb_streams; i++)
  {
    // look for the video stream
    if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && videoStreamIndex < 0)
    {
      videoStreamIndex = i;
    }

    // look for the audio stream
    if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioStreamIndex < 0)
    {
      audioStreamIndex = i;
    }
  }

  // return with error in case no video stream was found
  if (videoStreamIndex == -1)
  {
    std::cerr << "Could not open video stream" << std::endl;
    goto fail;
  }
  else
  {
    // open video stream
    ret = streamComponentOpen(videoState, videoStreamIndex);

    // check video codec was opened correctly
    if (ret < 0)
    {
      std::cerr << "Could not find video codec" << std::endl;
      goto fail;
    }

    m_videoRenderer = std::make_unique<VideoRenderer>();
    m_videoRenderer->start(videoState);
  }

  // return with error in case no audio stream was found
  if (audioStreamIndex == -1)
  {
    std::cerr << "Could not find audio stream" << std::endl;
    goto fail;
  }
  else
  {
    // open audio stream component codec
    ret = streamComponentOpen(videoState, audioStreamIndex);

    // check audio codec was opened correctly
    if (ret < 0)
    {
      std::cerr << "Could not find audio codec" << std::endl;
      goto fail;
    }
  }
  if (videoStreamIndex < 0 || audioStreamIndex < 0)
  {
    std::cerr << "Could not open codecs " << m_filename << std::endl;
    goto fail;
  }

  packet = av_packet_alloc();
  if (packet == nullptr)
  {
    std::cerr << "Could not alloc packet" << std::endl;
    goto fail;
  }

  // main decode loop. read in a packet and put it on the queue
  for (;;)
  {
    // seek stuff goes here
    auto& seekReq = videoState->seekRequest();
    if (seekReq)
    {
      int64_t seek_target_video = videoState->seek_pos;
      int64_t seek_target_audio = videoState->seek_pos;

      if (videoStreamIndex >= 0 && audioStreamIndex >= 0)
      {
        // MSVC does not support compound literals like AV_TIME_BASE_Q in C++ code (compiler error C4576)
        AVRational timebase;
        timebase.num = 1;
        timebase.den = AV_TIME_BASE;

        seek_target_video = av_rescale_q(
          seek_target_video
          , timebase
          , formatCtx->streams[videoStreamIndex]->time_base);
        seek_target_audio = av_rescale_q(
          seek_target_audio
          , timebase
          , formatCtx->streams[audioStreamIndex]->time_base);
      }

      ret = av_seek_frame(
        formatCtx
        , videoStreamIndex
        , seek_target_video
        , videoState->seek_flags);
      ret &= av_seek_frame(
        formatCtx
        , audioStreamIndex
        , seek_target_audio
        , videoState->seek_flags);

      if (ret >= 0)
      {
        if (videoStreamIndex >= 0)
        {
          videoState->videoq.flush();
          videoState->videoq.put(videoState->flush_pkt);
        }

        if (audioStreamIndex >= 0)
        {
          videoState->audioq.flush();
          videoState->audioq.put(videoState->flush_pkt);
        }
        videoState->seek_req = 0;
      }
    }

    // check audio and video packets queues size
    if (videoState->audioq.size + videoState->videoq.size > MAX_QUEUE_SIZE)
    {
      // wait for audio and video queues to decrease size
      SDL_Delay(10);
      continue;
    }
    // read data from the AVFormatContext by repeatedly calling av_read_frame
    ret = av_read_frame(formatCtx, packet);
    if (ret < 0)
    {
      if (ret == AVERROR_EOF)
      {
        // wait for the rest of the program to end
        while (videoState->videoq.nb_packets > 0 && videoState->audioq.nb_packets > 0)
        {
          SDL_Delay(10);
        }

        // media EOF reached, quit
        break;
      }
      else if (formatCtx->pb->error == 0)
      {
        // no read error, wait for user input
        SDL_Delay(10);
        continue;
      }
      else
      {
        // exit for loop in case of error
        break;
      }
    }

    // put the packet in the appropriate queue
    if (packet->stream_index == videoStreamIndex)
    {
      videoState->pushVideoPacketRead(packet);
    }
    else if (packet->stream_index == audioStreamIndex)
    {
      videoState->pushAudioPacketRead(packet);
    }
    else
    {
      // otherwise free the memory
      av_packet_unref(packet);
    }
  }

  // wait for the rest of the program to end
  while (!videoState->quit)
  {
    SDL_Delay(100);
  }

  return 0;
}

int VideoReader::streamComponentOpen(std::shared_ptr<VideoState> vs, const int& streamIndex)
{
  // retrieve file I/O context
  auto formatCtx = vs->formatCtx();

  // check the given stream index in valid
  if (streamIndex < 0 || streamIndex >= formatCtx->nb_streams)
  {
    std::cerr << "invalid stream index" << std::endl;
    return -1;
  }

  // retrieve codec for the given stream index
  const AVCodec* codec = avcodec_find_decoder(formatCtx->streams[streamIndex]->codecpar->codec_id);
  if (codec == nullptr)
  {
    std::cerr << "unsupported codec" << std::endl;
    return -1;
  }

  // retrieve codec context
  AVCodecContext* codecCtx = avcodec_alloc_context3(codec);

  // use multi core
  // fhd, 60p = 1 threads
  // 4k, 60p  = 4 threads
  //codecCtx->thread_count = 4;
  //codecCtx->thread_type = FF_THREAD_FRAME;

  int ret = avcodec_parameters_to_context(codecCtx, formatCtx->streams[streamIndex]->codecpar);
  if (ret != 0)
  {
    std::cerr << "Could not copy codec context" << std::endl;
    return -1;
  }

  if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO)
  {
    SDL_AudioSpec wants{};
    SDL_AudioSpec spec{};

    wants.freq = codecCtx->sample_rate;
    wants.format = AUDIO_S16SYS;
    wants.channels = codecCtx->ch_layout.nb_channels;
    wants.silence = 0;
    wants.samples = SDL_AUDIO_BUFFER_SIZE;
    wants.callback = audio_callback;
    wants.userdata = vs.get();

    // open audio device
    auto& outputAudioDeviceIndex = vs->outputAudioDeviceIndex();
    auto& sdlAudioDeviceID = vs->sdlAudioDeviceID();
    sdlAudioDeviceID = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(outputAudioDeviceIndex, 0), false, &wants, &spec, 0);
    if (sdlAudioDeviceID <= 0)
    {
      ret = -1;
      return -1;
    }
    vs->setSdlAudioDeviceID(sdlAudioDeviceID);
  }
  // init the AVCodecContext to use the given AVCodec
  if (avcodec_open2(codecCtx, codec, nullptr) < 0)
  {
    std::cerr << "unsupported codec" << std::endl;
    return -1;
  }

  switch (codecCtx->codec_type)
  {
    case AVMEDIA_TYPE_AUDIO:
    {
      // zero out the block of memory pointed
      std::memset(&vs->audio_pkt, 0, sizeof(vs->audio_pkt));

      // init audio pkt queue
      vs->audioq.init();

      // start playing audio device
      SDL_PauseAudioDevice(sdlAudioDeviceID, 0);
    }
    break;

    case AVMEDIA_TYPE_VIDEO:
    {
      // !!! Don't forget to init the frame timer
      // previous frame delay: 1ms = 1e-6s
      videoState->frame_timer = (double)av_gettime() / 1000000.0;
      videoState->frame_last_delay = 40e-3;
      videoState->video_current_pts_time = av_gettime();

      // init video packet queue
      videoState->videoq.init();

      // start video thread
      m_videoDecoder = new VideoDecoder();
      m_videoDecoder->start(videoState);

      // set up the videostate swscontext to convert the image data to yuv420
      videoState->sws_ctx = sws_getContext(
        videoState->video_ctx->width
        , videoState->video_ctx->height
        , videoState->video_ctx->pix_fmt
        , videoState->video_ctx->width
        , videoState->video_ctx->height
        , AV_PIX_FMT_YUV420P
        , SWS_BILINEAR
        , nullptr
        , nullptr
        , nullptr);
      // init sdl_surface mutex ref
      videoState->screen_mutex = SDL_CreateMutex();

    }
    break;

    default:
    {
      // nothing
    }
  }
  return 0;
}

int VideoReader::releaseAll()
{
  if (packet)
  {
    av_packet_free(&packet);
  }
  packet = nullptr;

  return 0;
}

