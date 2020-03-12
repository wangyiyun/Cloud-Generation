#pragma once
//Modified from the muxing.c example at https://ffmpeg.org/doxygen/trunk/muxing_8c-source.html

#define __STDC_CONSTANT_MACROS


extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/gl.h>


#define STREAM_DURATION   60.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC

// a wrapper around a single output AVStream
typedef struct OutputStream {
   AVStream *st;
   AVCodecContext *enc;
   /* pts of the next frame that will be generated */
   int64_t next_pts;
   int samples_count;
   AVFrame *frame;
   AVFrame *tmp_frame;
   float t, tincr, tincr2;
   struct SwsContext *sws_ctx;
   struct SwrContext *swr_ctx;
} OutputStream;

int main_test();

extern OutputStream video_st, audio_st;
extern AVOutputFormat *fmt;
extern AVFormatContext *oc;
extern AVCodec *audio_codec;
extern AVCodec *video_codec;
extern int have_video, have_audio;
extern int encode_video, encode_audio;
extern AVDictionary *opt;
extern uint8_t *rgb;
extern GLubyte *pixels;


int start_encoding(const char *filename, int width, int height);
void read_frame_to_encode(uint8_t **rgb, GLubyte **pixels, unsigned int width, unsigned int height);
void encode_frame(uint8_t *rgb);
void finish_encoding();