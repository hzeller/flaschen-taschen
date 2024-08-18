// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
//
// Quick hack based on ffmpeg
// tutorial http://dranger.com/ffmpeg/tutorial01.html
// in turn based on a tutorial by
// Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
//

// Ancient AV versions forgot to set this.
#define __STDC_CONSTANT_MACROS

// libav: "U NO extern C in header ?"
extern "C" {
#  include <libavcodec/avcodec.h>
#  include <libavdevice/avdevice.h>
#  include <libavformat/avformat.h>
#  include <libavutil/imgutils.h>
#  include <libswscale/swscale.h>
}

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "udp-flaschen-taschen.h"

typedef int64_t tmillis_t;

static tmillis_t GetTimeInMillis() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void add_nanos(struct timespec *accumulator, long nanoseconds) {
  accumulator->tv_nsec += nanoseconds;
  while (accumulator->tv_nsec > 1000000000) {
    accumulator->tv_nsec -= 1000000000;
    accumulator->tv_sec += 1;
  }
}

volatile bool interrupt_received = false;
static void InterruptHandler(int) {
  interrupt_received = true;
}

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#  define av_frame_alloc avcodec_alloc_frame
#  define av_frame_free avcodec_free_frame
#endif

bool PlayVideo(const char *filename, UDPFlaschenTaschen& display, int verbose, float repeatTimeout);

void SendFrame(AVFrame *pFrame, UDPFlaschenTaschen *display) {
    // Write pixel data
    const int height = display->height();
    for(int y = 0; y < height; ++y) {
        char *raw_buffer = (char*) &display->GetPixel(0, y); // Yes, I know :)
        memcpy(raw_buffer, pFrame->data[0] + y*pFrame->linesize[0],
               3 * display->width());
    }
    display->Send();
}

static int usage(const char *progname) {
    fprintf(stderr, "usage: %s [options] <video>\n", progname);
    fprintf(stderr, "Options:\n"
            "\t-g <width>x<height>[+<off_x>+<off_y>[+<layer>]] : Output geometry. Default 20x20+0+0\n"
            "\t-h <host>          : Flaschen-Taschen display hostname.\n"
            "\t-l <layer>         : Layer 0..15. Default 0 (note if also given in -g, then last counts)\n"
            "\t-t <repeat-secs>   : Loop until at least n seconds passed.\n"
            "\t-c                 : clear display/layer before close\n"
            "\t-v                 : verbose (multiple: more verbose).\n");
    return 1;
}

// Convert deprecated color formats to new and manually set the color range.
// YUV has funny ranges (16-235), while the YUVJ are 0-255. SWS prefers to
// deal with the YUV range, but then requires to set the output range.
// https://libav.org/documentation/doxygen/master/pixfmt_8h.html#a9a8e335cf3be472042bc9f0cf80cd4c5
SwsContext *CreateSWSContext(const AVCodecContext *codec_ctx,
                             int display_width, int display_height) {
  AVPixelFormat pix_fmt;
  bool src_range_extended_yuvj = true;
  // Remap deprecated to new pixel format.
  switch (codec_ctx->pix_fmt) {
  case AV_PIX_FMT_YUVJ420P: pix_fmt = AV_PIX_FMT_YUV420P; break;
  case AV_PIX_FMT_YUVJ422P: pix_fmt = AV_PIX_FMT_YUV422P; break;
  case AV_PIX_FMT_YUVJ444P: pix_fmt = AV_PIX_FMT_YUV444P; break;
  case AV_PIX_FMT_YUVJ440P: pix_fmt = AV_PIX_FMT_YUV440P; break;
  default:
    src_range_extended_yuvj = false;
    pix_fmt = codec_ctx->pix_fmt;
  }
  SwsContext *swsCtx = sws_getContext(codec_ctx->width, codec_ctx->height,
                                      pix_fmt,
                                      display_width, display_height,
                                      AV_PIX_FMT_RGB24, SWS_BILINEAR,
                                      NULL, NULL, NULL);
  if (src_range_extended_yuvj) {
    // Manually set the source range to be extended. Read modify write.
    int dontcare[4];
    int src_range, dst_range;
    int brightness, contrast, saturation;
    sws_getColorspaceDetails(swsCtx, (int**)&dontcare, &src_range,
                             (int**)&dontcare, &dst_range, &brightness,
                             &contrast, &saturation);
    const int* coefs = sws_getCoefficients(SWS_CS_DEFAULT);
    src_range = 1;  // New src range.
    sws_setColorspaceDetails(swsCtx, coefs, src_range, coefs, dst_range,
                             brightness, contrast, saturation);
  }
  return swsCtx;
}

int main(int argc, char *argv[]) {
    int display_width = 45;
    int display_height = 35;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    float repeatTimeout = 0;
    int verbose = 0;
    bool clear_after = false;
    const char *ft_host = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "g:h:t:cvl:")) != -1) {
        switch (opt) {
        case 'g':
            if (sscanf(optarg, "%dx%d%d%d%d",
                       &display_width, &display_height, &off_x, &off_y, &off_z) < 2) {
                fprintf(stderr, "Invalid size spec '%s'", optarg);
                return usage(argv[0]);
            }
            break;
        case 'h':
            ft_host = strdup(optarg); // leaking. Ignore.
            break;
        case 'l':
            if (sscanf(optarg, "%d", &off_z) != 1 || off_z < 0 || off_z >= 16) {
                fprintf(stderr, "Invalid layer '%s'\n", optarg);
                return usage(argv[0]);
            }
            break;
        case 't':
            repeatTimeout = atof(optarg);
            break;
        case 'c':
            clear_after = true;
            break;
        case 'v':
            verbose++;
            break;
        default:
            return usage(argv[0]);
        }
    }


    if (optind >= argc) {
        fprintf(stderr, "Expected image filename.\n");
        return usage(argv[0]);
    }

    const int ft_socket = OpenFlaschenTaschenSocket(ft_host);
    if (ft_socket < 0) {
        fprintf(stderr, "Couldn't open socket to FlaschenTaschen; did you provide correct hostname with -h <hostname> ?\n");
        return -1;
    }
    UDPFlaschenTaschen display(ft_socket, display_width, display_height);
    display.SetOffset(off_x, off_y, off_z);

    // Register all formats and codecs
#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(58, 9, 100)
    av_register_all();
#endif
    avdevice_register_all();
    avformat_network_init();

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    int numberPlayed = 0;
    for (int imgarg = optind; imgarg < argc && !interrupt_received; ++imgarg) {
        const char *movie_file = argv[imgarg];

        bool playresult = PlayVideo(movie_file, display, verbose, repeatTimeout);
        if (playresult)
            numberPlayed++;

        if (interrupt_received) {
            // Feedback for Ctrl-C, but most importantly, force a newline
            // at the output, so that commandline-shell editing is not messed up.
            fprintf(stderr, "Got interrupt. Exiting\n");
            break;
        }
    }
    if (off_z > 0 || clear_after) {
        display.Clear();
        display.Send();
    }
    return !numberPlayed;
}

/// @returns true on video successfully played-through
bool PlayVideo(const char *filename, UDPFlaschenTaschen& display, int verbose, float repeatTimeout) {
    // Open video file
    AVFormatContext *format_context = avformat_alloc_context();
    if (avformat_open_input(&format_context, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Can't open file %s\n", filename);
        return false;
    }
    fprintf(stderr, "Playing %s\n", filename);

    // Retrieve stream information
    if (avformat_find_stream_info(format_context, NULL) < 0) {
        fprintf(stderr, "Can't open stream for %s\n", filename);
        return false;
    }

    // Dump information about file onto standard error
    if (verbose > 1) {
        av_dump_format(format_context, 0, filename, 0);
    }

    // Find the first video stream
    int videoStream = -1;
    const AVCodecParameters *codec_parameters = NULL;
    const AVCodec *av_codec = NULL;
    for (int i=0; i < (int)format_context->nb_streams; ++i) {
        codec_parameters = format_context->streams[i]->codecpar;
        av_codec = avcodec_find_decoder(codec_parameters->codec_id);
        if (!av_codec) continue;
        if (codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        fprintf(stderr, "Can't open stream for %s\n", filename);
        return false;
    }

    // Get a pointer to the codec context for the video stream
    AVStream *const stream = format_context->streams[videoStream];
    AVRational rate = av_guess_frame_rate(format_context, stream, NULL);
    const long frame_wait_nanos = 1e9 * rate.den / rate.num;
    if (verbose > 1) {
        fprintf(stderr, "FPS: %f\n", 1.0*rate.num / rate.den);
    }

    AVCodecContext *codec_context = avcodec_alloc_context3(av_codec);
    if (avcodec_parameters_to_context(codec_context, codec_parameters) < 0) {
        return false;
    }

    if (avcodec_open2(codec_context, av_codec, NULL) < 0) {
        return false; // Could not open codec
    }

    // Allocate an AVFrame structure with data that we can send out.
    AVFrame *output_frame = av_frame_alloc();
    if (!output_frame ||
        av_image_alloc(output_frame->data, output_frame->linesize,
                       display.width(), display.height(), AV_PIX_FMT_RGB24,
                       64) < 0) {
      return false;
    }

    // initialize SWS context for software scaling
    SwsContext* sws_ctx = CreateSWSContext(codec_context,
                                           display.width(), display.height());
    if (!sws_ctx) {
        fprintf(stderr, "Trouble doing scaling to %dx%d :(\n",
                display.width(), display.height());
        return false;
    }

    // Read frames and send to FlaschenTaschen.
    const tmillis_t startTime = GetTimeInMillis();

    struct timespec next_frame;
    AVPacket *packet = av_packet_alloc();
    AVFrame *decode_frame = av_frame_alloc();  // Decode video into this
    long frame_count = 0, repeated_count = 0;
    while (!interrupt_received) {
        clock_gettime(CLOCK_MONOTONIC, &next_frame);

        frame_count = 0;
        while (!interrupt_received && av_read_frame(format_context, packet) >= 0) {
            // Is this a packet from the video stream?
            if (packet->stream_index == videoStream) {
                // Decode video frame
                if (avcodec_send_packet(codec_context, packet) < 0)
                    continue;

                if (avcodec_receive_frame(codec_context, decode_frame) < 0)
                    continue;

                // Absolute end of this frame now so that we don't include
                // decoding and sending overhead.
                add_nanos(&next_frame, frame_wait_nanos);

                // Convert the image from its native format to RGB
                sws_scale(sws_ctx, (uint8_t const * const *)decode_frame->data,
                          decode_frame->linesize, 0, codec_context->height,
                          output_frame->data, output_frame->linesize);

                // Save the frame to disk
                SendFrame(output_frame, &display);
                frame_count++;
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame,
                                NULL);
            }

            // Free the packet that was allocated by av_read_frame
            av_packet_unref(packet);
        }
        repeated_count++; //if time allows- keep playing

        const tmillis_t elapsed = GetTimeInMillis() - startTime;
        if (elapsed >= repeatTimeout * 1000)
            break;

        av_seek_frame(format_context, -1, 1, AVSEEK_FLAG_FRAME); //start playing from the beginning
        if (verbose > 1)
            fprintf(stderr, "loop %ld done after %0.1fs (%ld frames)\n", repeated_count, elapsed / 1000.0, frame_count);
    }

    av_packet_free(&packet);

    av_frame_free(&output_frame);
    av_frame_free(&decode_frame);
    avcodec_close(codec_context);
    avformat_close_input(&format_context);

    if (verbose) {
      const float total_time = (GetTimeInMillis() - startTime) / 1000.0;
      fprintf(stderr,
              "Finished playing %ld frames %ld times for %0.1fs total (%.1f "
              "fps avg)\n",
              frame_count, repeated_count, total_time,
              repeated_count * frame_count / total_time);
    }

    return !interrupt_received;
}
