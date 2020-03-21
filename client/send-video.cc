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
#  include <libavformat/avformat.h>
#  include <libswscale/swscale.h>
}

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>

#include "udp-flaschen-taschen.h"

typedef int64_t tmillis_t;

static tmillis_t GetTimeInMillis() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
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
            "\t-t <repeat-secs>   : Repeat for at least n seconds\n"
            "\t-c                 : clear display/layer before close\n"
            "\t-v                 : verbose.\n");
    return 1;
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
        fprintf(stderr, "Couldn't open socket to FlaschenTaschen\n");
        return -1;
    }
    UDPFlaschenTaschen display(ft_socket, display_width, display_height);
    display.SetOffset(off_x, off_y, off_z);

    // Register all formats and codecs
    av_register_all();
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
    AVFormatContext* pFormatCtx = NULL;
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        fprintf(stderr, "Can't open file %s\n", filename);
        return false;
    }
    fprintf(stderr, "Playing %s\n", filename);

    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        fprintf(stderr, "Can't open stream for %s\n", filename);
        return false;
    }

    // Dump information about file onto standard error
    if (verbose > 1) {
        av_dump_format(pFormatCtx, 0, filename, 0);
    }

    // Find the first video stream
    int videoStream = -1;
    for (int i=0; i < (int)pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    }
    if (videoStream==-1){
        fprintf(stderr, "Can't open stream for %s\n", filename);
        return false;
    }

    // Get a pointer to the codec context for the video stream
    AVCodecContext* pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    double fps = av_q2d(pFormatCtx->streams[videoStream]->avg_frame_rate);
    if (fps < 0) {
        fps = 1.0 / av_q2d(pFormatCtx->streams[videoStream]->codec->time_base);
    }
    if (verbose > 1) fprintf(stderr, "FPS: %f\n", fps);

    // Find the decoder for the video stream
    AVCodec* pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return false; // Codec not found
    }
    // Copy context
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
        fprintf(stderr, "Couldn't copy codec context");
        return false; // Error copying codec context
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return false; // Could not open codec

    // Allocate video frame
    AVFrame* pFrame = av_frame_alloc();

    // Allocate an AVFrame structure
    AVFrame* pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
        return false;

    // Determine required buffer size and allocate buffer
    int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24,
                                      display.width(), display.height());
    uint8_t* buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24,
                   display.width(), display.height());

    // initialize SWS context for software scaling
    struct SwsContext* sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                display.width(), display.height(),
                                                AV_PIX_FMT_RGB24,
                                                SWS_BILINEAR,
                                                NULL, NULL, NULL );
    if (sws_ctx == 0) {
        fprintf(stderr, "Trouble doing scaling to %dx%d :(\n", display.width(), display.height());
        return false;
    }

    // Read frames and send to FlaschenTaschen.
    const int frame_wait_micros = 1e6 / fps;
    const tmillis_t  startTime = GetTimeInMillis();
    long frame_count = 0, repeated_count = 0;
    while (!interrupt_received) {
        frame_count = 0;
        AVPacket packet;
        while (!interrupt_received && av_read_frame(pFormatCtx, &packet) >= 0) {
            // Is this a packet from the video stream?
            if (packet.stream_index == videoStream) {
                int frameFinished = 0;
                // Decode video frame
                avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

                // Did we get a video frame?
                if (frameFinished) {
                    // Convert the image from its native format to RGB
                    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                              pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGB->data, pFrameRGB->linesize);

                    // Save the frame to disk
                    SendFrame(pFrameRGB, &display);
                    frame_count++;
                }
                usleep(frame_wait_micros);
            }

            // Free the packet that was allocated by av_read_frame
            av_free_packet(&packet);
        }
        repeated_count++; //if time allows- keep playing

        const tmillis_t elapsed = GetTimeInMillis() - startTime;
        if (elapsed >= repeatTimeout * 1000)
            break;

        av_seek_frame(pFormatCtx, -1, 1, AVSEEK_FLAG_FRAME); //start playing from the beginning
        if (verbose > 1)
            fprintf(stderr, "loop %ld done after %0.1fs (%ld frames)\n", repeated_count, elapsed / 1000.0, frame_count);
    }

    // Free the RGB image
    av_free(buffer);
    av_frame_free(&pFrameRGB);

    // Free the YUV frame
    av_frame_free(&pFrame);

    // Close the codecs
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // Close the video file
    avformat_close_input(&pFormatCtx);
    if (verbose)
        fprintf(stderr, "Finished playing %ld frames %ld times for %0.1fs total\n", frame_count, repeated_count, (GetTimeInMillis() - startTime) / 1000.);
    return !interrupt_received;
}
