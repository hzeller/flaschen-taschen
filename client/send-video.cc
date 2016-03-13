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

#include "udp-flaschen-taschen.h"

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#  define av_frame_alloc avcodec_alloc_frame
#  define av_frame_free avcodec_free_frame
#endif

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
            "\t-v                 : verbose.\n");
    return 1;
}

int main(int argc, char *argv[]) {
    int display_width = 20;
    int display_height = 20;
    int off_x = 0;
    int off_y = 0;
    int off_z = 0;
    bool verbose = false;
    const char *ft_host = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "g:h:vl:")) != -1) {
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
            off_z = atoi(optarg);
            break;
        case 'v':
            verbose = true;
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

    // Initalizing these to NULL prevents segfaults!
    AVFormatContext   *pFormatCtx = NULL;
    int               i, videoStream;
    AVCodecContext    *pCodecCtxOrig = NULL;
    AVCodecContext    *pCodecCtx = NULL;
    AVCodec           *pCodec = NULL;
    AVFrame           *pFrame = NULL;
    AVFrame           *pFrameRGB = NULL;
    AVPacket          packet;
    int               frameFinished;
    int               numBytes;
    uint8_t           *buffer = NULL;
    struct SwsContext *sws_ctx = NULL;

    const char *movie_file = argv[optind];

    // Register all formats and codecs
    av_register_all();

    // Open video file
    if(avformat_open_input(&pFormatCtx, movie_file, NULL, NULL)!=0)
        return -1; // Couldn't open file

    // Retrieve stream information
    if(avformat_find_stream_info(pFormatCtx, NULL)<0)
        return -1; // Couldn't find stream information

    // Dump information about file onto standard error
    if (verbose) {
        av_dump_format(pFormatCtx, 0, movie_file, 0);
    }

    // Find the first video stream
    videoStream=-1;
    for(i=0; i < (int)pFormatCtx->nb_streams; ++i)
        if (pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
            videoStream=i;
            break;
        }
    if(videoStream==-1)
        return -1; // Didn't find a video stream

    // Get a pointer to the codec context for the video stream
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    double fps = av_q2d(pFormatCtx->streams[videoStream]->avg_frame_rate);
    if (fps < 0) {
        fps = 1.0 / av_q2d(pFormatCtx->streams[videoStream]->codec->time_base);
    }
    if (verbose) fprintf(stderr, "FPS: %f\n", fps);

    // Find the decoder for the video stream
    pCodec=avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if(pCodec==NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }
    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
        fprintf(stderr, "Couldn't copy codec context");
        return -1; // Error copying codec context
    }

    // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, NULL)<0)
        return -1; // Could not open codec

    // Allocate video frame
    pFrame=av_frame_alloc();

    // Allocate an AVFrame structure
    pFrameRGB=av_frame_alloc();
    if(pFrameRGB==NULL)
        return -1;

    // Determine required buffer size and allocate buffer
    numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                                pCodecCtx->height);
    buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                   pCodecCtx->width, pCodecCtx->height);

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
                             pCodecCtx->height,
                             pCodecCtx->pix_fmt,
                             display.width(), display.height(),
                             PIX_FMT_RGB24,
                             SWS_BILINEAR,
                             NULL,
                             NULL,
                             NULL
                             );
    if (sws_ctx == 0) {
        fprintf(stderr, "Trouble doing scaling to %dx%d :(\n",
                display.width(), display.height());
        return 1;
    }

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    // Read frames and send to FlaschenTaschen.
    const int frame_wait_micros = 1e6 / fps;
    while (!interrupt_received && av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index==videoStream) {
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
            }
            usleep(frame_wait_micros);
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    if (interrupt_received) {
        // Feedback for Ctrl-C, but most importantly, force a newline
        // at the output, so that commandline-shell editing is not messed up.
        fprintf(stderr, "Got interrupt. Exiting\n");
    }

    if (off_z > 0) {
        display.Clear();
        display.Send();
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

    return 0;
}
