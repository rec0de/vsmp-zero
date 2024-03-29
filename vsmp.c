// Configuration params in vsmp.h

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <signal.h>
#include <unistd.h>
#include "vsmp.h"
#include "dither.c"

#if DRYRUN != 1
  // Change this include if you're using a custom display driver
  #include "displays/genericIT8951.c"
#else
  #include "displays/dryrun.c"
#endif

#if LIGHSENSE == 1
  #include <bcm2835.h>
#endif

static void displayFrame(
  int64_t timestamp,
  int streamIdx,
  AVFormatContext *formatCtx,
  AVCodecContext *codecCtx,
  AVPacket *packet,
  AVFrame *frame
);

static void processFrame(unsigned char *frameBuf, int linesize, int width, int height);

static int lightsense();

static void backupProgress(int frame);

// Duration of one frame in AV_TIME_BASE units
int64_t timeBase;
int target = 0;

void cleanup() {
  printf("Shutting down");
  teardownDisplay();
  exit(0);
}

// libav code patched together from multiple sources,
// most importantly https://github.com/leandromoreira/ffmpeg-libav-tutorial/blob/master/0_hello_world.c
int main(int argc, const char *argv[]) {

  if (argc == 2) {
    printf("Attempting to read vsmp-index file\n");
    FILE *f = fopen("vsmp-index", "r");
    char fidx[16];
    fgets(fidx, 16, f);
    target = atoi(fidx);
    fclose(f);
    printf("Resuming playback at frame %d\n", target);
  }
  else if (argc == 3) {
    target = atoi(argv[2]);
  }
  else {
    printf("Usage: vsmp [video file] [frame index]\n");
    return -1;
  }

  if(initDisplay()) {
    printf("Display init error \n");
    return 1;
  }
  printf("Display initialized\n");
  
  // AVFormatContext holds the header information from the format (Container)
  // http://ffmpeg.org/doxygen/trunk/structAVFormatContext.html
  AVFormatContext *pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    printf("ERROR could not allocate memory for Format Context");
    return -1;
  }

  if (avformat_open_input(&pFormatContext, argv[1], NULL, NULL) != 0) {
    printf("ERROR could not open the file");
    return -1;
  }

  if (avformat_find_stream_info(pFormatContext,  NULL) < 0) {
    printf("ERROR could not get the stream info");
    return -1;
  }

  AVCodec *pCodec = NULL;
  AVCodecParameters *pCodecParameters =  NULL;
  int video_stream_index = -1;

  // loop though all the streams and print its main information
  for (int i = 0; i < pFormatContext->nb_streams; i++) {
    AVCodecParameters *pLocalCodecParameters =  NULL;
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

    // get first video stream
    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
      video_stream_index = i;

      #if HWACCEL
        pCodec = avcodec_find_decoder_by_name("h264_mmal");
      #else
        pCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);
      #endif
      
      pCodecParameters = pLocalCodecParameters;
    } 
  }

  // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
  AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
  if (!pCodecContext) {
    printf("failed to allocated memory for AVCodecContext");
    return -1;
  }

  // Fill the codec context based on the values from the supplied codec parameters
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
  if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0) {
    printf("failed to copy codec params to codec context");
    return -1;
  }

  // Initialize the AVCodecContext to use the given AVCodec.
  // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
  if (avcodec_open2(pCodecContext, pCodec, NULL) < 0) {
    printf("failed to open codec through avcodec_open2");
    return -1;
  }

  AVStream *vidstream;
  vidstream = pFormatContext->streams[video_stream_index];
  timeBase = (vidstream->time_base.den * vidstream->r_frame_rate.den) / (vidstream->time_base.num * vidstream->r_frame_rate.num);

  // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
  AVFrame *pFrame = av_frame_alloc();
  if (!pFrame) {
    printf("failed to allocated memory for AVFrame");
    return -1;
  }
  
  // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
  AVPacket *pPacket = av_packet_alloc();
  if (!pPacket) {
    printf("failed to allocated memory for AVPacket");
    return -1;
  }

  #if LIGHSENSE
    if (!bcm2835_init()) {
      printf("bcm2835_init error (lightsense init) \n");
      return 1;
    }
    printf("Lightsense init done \n");
  #endif

  // INIT DONE
  printf("FFmpeg init done\n");

  clearDisplay();
  printf("Display cleared \n");

  int64_t timestamp = target * timeBase;
  clock_t loopstart;
  uint8_t consecutivePaints = 0;

  signal(SIGINT, cleanup);

  while(timestamp < pFormatContext->duration) {
    loopstart = clock();

    #if LIGHSENSE
      if(lightsense()) {
        displayFrame(timestamp, video_stream_index, pFormatContext, pCodecContext, pPacket, pFrame);  
      }
    #else
      displayFrame(timestamp, video_stream_index, pFormatContext, pCodecContext, pPacket, pFrame);
    #endif

    consecutivePaints++;
    target += FRAME_STEP_SIZE;
    timestamp = target * timeBase;

    if(consecutivePaints % 32 == 0)
      backupProgress(target);

    int timeSpent = (clock() - loopstart) / CLOCKS_PER_SEC;
    int sleepTime = (3600 / FRAMES_PER_HOUR) - timeSpent;
    if(sleepTime > 0)
      sleep(sleepTime);
  }

  // CLEANUP

  teardownDisplay();

  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pCodecContext);
  return 0;
}

static void displayFrame(
  int64_t timestamp,
  int streamIdx,
  AVFormatContext *formatCtx,
  AVCodecContext *codecCtx,
  AVPacket *packet,
  AVFrame *frame
) {

  // Seek to closest preceeding i-frame
  // This may not be necessary (and actually inefficient) when playing continuously frame-by-frame
  // but it keeps us from having to worry too much about out-of-order frames being lost
  av_seek_frame(formatCtx, streamIdx, timestamp, AVSEEK_FLAG_BACKWARD);

  char breakflag = 0;
  int status = av_read_frame(formatCtx, packet);

  // For some reason, the mmal decoder freaks out every now and then (~ once per week?), which I believe causes an endless loop here
  // So if we get an unknown decoding error, we'll just skip this frame
  while (status >= 0 && status != AVERROR_UNKNOWN) {
    if (packet->stream_index == streamIdx) {
      int response = avcodec_send_packet(codecCtx, packet);

      // One packet may contain multiple frames
      while (response >= 0) {
        response = avcodec_receive_frame(codecCtx, frame);
       
        // Frames may arrive out-of-order, so we'll check that we find a reasonably close match
        if(frame->pts >= timestamp && frame->pts <= timestamp + timeBase * 2) {
          processFrame(frame->data[0], frame->linesize[0], frame->width, frame->height);
          breakflag = 1;
          break;
        }
      }

      if(breakflag)
        break;
    }
    av_packet_unref(packet);
    status = av_read_frame(formatCtx, packet);
  }
}

// Performs simple white value adjustment
// Everything above newWhite in the buffer will be plain white
static unsigned char contrastAdjust(unsigned char newWhite, unsigned char pixel) {
  unsigned int in = pixel;
  unsigned int out = (in << 8)/(newWhite);
  if(out > 255)
    return 255;
  else
    return (unsigned char) out;
}

static void contrastAdjustBuffer(unsigned char *frameBuf, int linesize, int width, int height) {
  uint32_t i,j,idx;

  for(j = 0; j < height; j++) {
    for(i = 0; i < width; i++) {
      idx = j * linesize + i;
      frameBuf[idx] = contrastAdjust(WHITE_VALUE, frameBuf[idx]);
    }
  }
}

static void processFrame(unsigned char *frameBuf, int linesize, int width, int height) {
  contrastAdjustBuffer(frameBuf, linesize, width, height);
  DITHER(frameBuf, linesize, width, height);
  pixelPush(frameBuf, linesize, width, height);
}

static void backupProgress(int target) {
  FILE *f;
  f = fopen("vsmp-index", "w");
  fprintf(f, "%d\n", target);
  fclose(f);
}

#if LIGHSENSE

  // Adapted from https://pimylifeup.com/raspberry-pi-light-sensor/
  static int lightsense() {

    // tie the lighsense pin low to drain the capacitor
    bcm2835_gpio_fsel(LIGHTSENSE_GPIO_PIN, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr(LIGHTSENSE_GPIO_PIN);
    sleep(0.1);
    
    // switch the lighsense pin to input
    bcm2835_gpio_set_pud(LIGHTSENSE_GPIO_PIN, BCM2835_GPIO_PUD_OFF);
    bcm2835_gpio_fsel(LIGHTSENSE_GPIO_PIN, BCM2835_GPIO_FSEL_INPT);

    uint8_t data = 0;
    int count = 0;

    // measure how long it takes for the capacitor to charge
    while(data == 0 && count < LIGHTSENSE_THRESHOLD) {
        data = bcm2835_gpio_lev(LIGHTSENSE_GPIO_PIN);
        count++;
    }

    return count != LIGHTSENSE_THRESHOLD;
  }

#endif