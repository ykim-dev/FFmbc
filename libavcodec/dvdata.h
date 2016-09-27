/*
 * Constants for DV codec
 * Copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation;
 * version 2 of the License.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Constants for DV codec.
 */

#ifndef AVCODEC_DVDATA_H
#define AVCODEC_DVDATA_H

#include "libavutil/rational.h"
#include "avcodec.h"
#include "dsputil.h"
#include "get_bits.h"

typedef struct DVwork_chunk {
    uint16_t  buf_offset;
    uint16_t  mb_coordinates[5];
} DVwork_chunk;

/*
 * DVprofile is used to express the differences between various
 * DV flavors. For now it's primarily used for differentiating
 * 525/60 and 625/50, but the plans are to use it for various
 * DV specs as well (e.g. SMPTE314M vs. IEC 61834).
 */
typedef struct DVprofile {
    int              dsf;                   /* value of the dsf in the DV header */
    int              video_stype;           /* stype for VAUX source pack */
    int              frame_size;            /* total size of one frame in bytes */
    int              difseg_size;           /* number of DIF segments per DIF channel */
    int              n_difchan;             /* number of DIF channels per frame */
    AVRational       time_base;             /* 1/framerate */
    int              ltc_divisor;           /* FPS from the LTS standpoint */
    int              height;                /* picture height in pixels */
    int              width;                 /* picture width in pixels */
    AVRational       sar[2];                /* sample aspect ratios for 4:3 and 16:9 */
    DVwork_chunk    *work_chunks;           /* each thread gets its own chunk of frame to work on */
    uint32_t        *idct_factor;           /* set of iDCT factor tables */
    enum PixelFormat pix_fmt;               /* picture pixel format */
    int              bpm;                   /* blocks per macroblock */
    const uint8_t   *block_sizes;           /* AC block sizes, in bits */
    int              audio_stride;          /* size of audio_shuffle table */
    int              audio_min_samples[3];  /* min amount of audio samples */
                                            /* for 48kHz, 44.1kHz and 32kHz */
    int              audio_samples_dist[5]; /* how many samples are supposed to be */
                                            /* in each frame in a 5 frames window */
    const uint8_t  (*audio_shuffle)[9];     /* PCM shuffling table */
} DVprofile;

typedef struct DVVideoContext {
    const DVprofile *sys;
    AVFrame          picture;
    AVCodecContext  *avctx;
    uint8_t         *buf;

    uint8_t  dv_zigzag[2][64];

    DSPContext dsp;
    void (*fdct[2])(DCTELEM *block);
    void (*idct_put[2])(uint8_t *dest, int line_size, DCTELEM *block);
} DVVideoContext;

/* unquant tables (not used directly) */
static const uint8_t dv_quant_shifts[22][4] = {
  { 3,3,4,4 },
  { 3,3,4,4 },
  { 2,3,3,4 },
  { 2,3,3,4 },
  { 2,2,3,3 },
  { 2,2,3,3 },
  { 1,2,2,3 },
  { 1,2,2,3 },
  { 1,1,2,2 },
  { 1,1,2,2 },
  { 0,1,1,2 },
  { 0,1,1,2 },
  { 0,0,1,1 },
  { 0,0,1,1 },
  { 0,0,0,1 },
  { 0,0,0,0 },
  { 0,0,0,0 },
  { 0,0,0,0 },
  { 0,0,0,0 },
  { 0,0,0,0 },
  { 0,0,0,0 },
  { 0,0,0,0 },
};

static const uint8_t dv_quant_offset[4] = { 6,  3,  0,  1 };
static const uint8_t dv_quant_areas[4]  = { 6, 21, 43, 64 };

/* setting this to 1 results in a faster codec but
 * somewhat lower image quality */
#define DV100_SACRIFICE_QUALITY_FOR_SPEED 1

/* quantization quanta by QNO for DV100 */
static const uint8_t dv100_qstep[16] = {
    1, /* QNO = 0 and 1 both have no quantization */
    1,
    2, 3, 4, 5, 6, 7, 8, 16, 18, 20, 22, 24, 28, 52
};

/* pack combination of QNO and CNO into a single 8-bit value */
#define DV100_MAKE_QLEVEL(qno,cno) ((qno<<2) | (cno))
#define DV100_QLEVEL_QNO(qlevel) (qlevel>>2)
#define DV100_QLEVEL_CNO(qlevel) (qlevel&0x3)

/* The quantization step is determined by a combination of QNO and
   CNO. We refer to these combinations as "qlevels" (this term is our
   own, it's not mentioned in the spec). We use CNO, a multiplier on
   the quantization step, to "fill in the gaps" between quantization
   steps associated with successive values of QNO. e.g. there is no
   QNO for a quantization step of 10, but we can use QNO=5 CNO=1 to
   get the same result. The table below encodes combinations of QNO
   and CNO in order of increasing quantization coarseness. */

static const uint8_t dv100_qlevels[] = {
    DV100_MAKE_QLEVEL( 1,0), //  1*1= 1
    DV100_MAKE_QLEVEL( 1,0), //  1*1= 1
    DV100_MAKE_QLEVEL( 2,0), //  2*1= 2
    DV100_MAKE_QLEVEL( 3,0), //  3*1= 3
    DV100_MAKE_QLEVEL( 4,0), //  4*1= 4
    DV100_MAKE_QLEVEL( 5,0), //  5*1= 5
    DV100_MAKE_QLEVEL( 6,0), //  6*1= 6
    DV100_MAKE_QLEVEL( 7,0), //  7*1= 7
    DV100_MAKE_QLEVEL( 8,0), //  8*1= 8
    DV100_MAKE_QLEVEL( 5,1), //  5*2=10
    DV100_MAKE_QLEVEL( 6,1), //  6*2=12
    DV100_MAKE_QLEVEL( 7,1), //  7*2=14
    DV100_MAKE_QLEVEL( 9,0), // 16*1=16
    DV100_MAKE_QLEVEL(10,0), // 18*1=18
    DV100_MAKE_QLEVEL(11,0), // 20*1=20
    DV100_MAKE_QLEVEL(12,0), // 22*1=22
    DV100_MAKE_QLEVEL(13,0), // 24*1=24
    DV100_MAKE_QLEVEL(14,0), // 28*1=28
    DV100_MAKE_QLEVEL( 9,1), // 16*2=32
    DV100_MAKE_QLEVEL(10,1), // 18*2=36
    DV100_MAKE_QLEVEL(11,1), // 20*2=40
    DV100_MAKE_QLEVEL(12,1), // 22*2=44
    DV100_MAKE_QLEVEL(13,1), // 24*2=48
    DV100_MAKE_QLEVEL(15,0), // 52*1=52
    DV100_MAKE_QLEVEL(14,1), // 28*2=56
    DV100_MAKE_QLEVEL( 9,2), // 16*4=64
    DV100_MAKE_QLEVEL(10,2), // 18*4=72
    DV100_MAKE_QLEVEL(11,2), // 20*4=80
    DV100_MAKE_QLEVEL(12,2), // 22*4=88
    DV100_MAKE_QLEVEL(13,2), // 24*4=96
    // ...
    DV100_MAKE_QLEVEL(15,3), // 52*8=416
};

static const int dv100_num_qlevels = sizeof(dv100_qlevels)/sizeof(dv100_qlevels[0]);

/* how much to increase qlevel when we need to compress more coarsely */
/* this is a tradeoff between encoding speed and space efficiency */
/* the highest-quality, lowest-speed option it to use 1 for all qlevels. */
static const uint8_t dv100_qstep_delta[16] = {
#if DV100_SACRIFICE_QUALITY_FOR_SPEED
    0, 2, 0, 5, 0, 0, 0, 0, 1, 6, 0, 0, 0, 0, 0, 0,
#else
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
#endif
};

/* how much to decrease qlevel when we can compress more finely */
/* must be the "inverse" of dv100_qstep_delta */
static const uint8_t dv100_qbackstep_delta[16] = {
#if DV100_SACRIFICE_QUALITY_FOR_SPEED
    0, 0, 0, 2, 0, 0, 0, 0, 5, 1, 0, 0, 0, 0, 0, 6,
#else
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
#endif
};

static const int dv100_min_bias = 0;
static const int dv100_chroma_bias = 0;
static const int dv100_starting_qno = 1;
static const int dv100_min_qno = 1;

#if DV100_SACRIFICE_QUALITY_FOR_SPEED
static const int dv100_qlevel_inc = 4;
#else
static const int dv100_qlevel_inc = 1;
#endif

// 1/qstep, shifted up by 16 bits
static const int dv100_qstep_bits = 16;
static const int dv100_qstep_inv[16] = {
        65536,  65536,  32768,  21845,  16384,  13107,  10923,  9362,  8192,  4096,  3641,  3277,  2979,  2731,  2341,  1260,
};

/* DV25/50 DCT coefficient weights and inverse weights */
/* created by dvtables.py */
static const int dv_weight_bits = 18;
static const int dv_weight_88[64] = {
 131072, 257107, 257107, 242189, 252167, 242189, 235923, 237536,
 237536, 235923, 229376, 231390, 223754, 231390, 229376, 222935,
 224969, 217965, 217965, 224969, 222935, 200636, 218652, 211916,
 212325, 211916, 218652, 200636, 188995, 196781, 205965, 206433,
 206433, 205965, 196781, 188995, 185364, 185364, 200636, 200704,
 200636, 185364, 185364, 174609, 180568, 195068, 195068, 180568,
 174609, 170091, 175557, 189591, 175557, 170091, 165371, 170627,
 170627, 165371, 160727, 153560, 160727, 144651, 144651, 136258,
};
static const int dv_weight_248[64] = {
 131072, 242189, 257107, 237536, 229376, 200636, 242189, 223754,
 224969, 196781, 262144, 242189, 229376, 200636, 257107, 237536,
 211916, 185364, 235923, 217965, 229376, 211916, 206433, 180568,
 242189, 223754, 224969, 196781, 211916, 185364, 235923, 217965,
 200704, 175557, 222935, 205965, 200636, 185364, 195068, 170627,
 229376, 211916, 206433, 180568, 200704, 175557, 222935, 205965,
 175557, 153560, 188995, 174609, 165371, 144651, 200636, 185364,
 195068, 170627, 175557, 153560, 188995, 174609, 165371, 144651,
};
static const int dv_iweight_bits = 14;
static const int dv_iweight_88[64] = {
 32768, 16710, 16710, 17735, 17015, 17735, 18197, 18079,
 18079, 18197, 18725, 18559, 19196, 18559, 18725, 19284,
 19108, 19692, 19692, 19108, 19284, 21400, 19645, 20262,
 20214, 20262, 19645, 21400, 22733, 21845, 20867, 20815,
 20815, 20867, 21845, 22733, 23173, 23173, 21400, 21400,
 21400, 23173, 23173, 24600, 23764, 22017, 22017, 23764,
 24600, 25267, 24457, 22672, 24457, 25267, 25971, 25191,
 25191, 25971, 26715, 27962, 26715, 29642, 29642, 31536,
};
static const int dv_iweight_248[64] = {
 32768, 17735, 16710, 18079, 18725, 21400, 17735, 19196,
 19108, 21845, 16384, 17735, 18725, 21400, 16710, 18079,
 20262, 23173, 18197, 19692, 18725, 20262, 20815, 23764,
 17735, 19196, 19108, 21845, 20262, 23173, 18197, 19692,
 21400, 24457, 19284, 20867, 21400, 23173, 22017, 25191,
 18725, 20262, 20815, 23764, 21400, 24457, 19284, 20867,
 24457, 27962, 22733, 24600, 25971, 29642, 21400, 23173,
 22017, 25191, 24457, 27962, 22733, 24600, 25971, 29642,
};

/* DV100 weights are pre-zigzagged, inverted and multiplied by 2^(dv100_weight_shift)
   (in DV100 the AC components are divided by the spec weights) */
static const int dv100_weight_shift = 16;
static const int dv_weight_1080[2][64] = {
    { 8192, 65536, 65536, 61681, 61681, 61681, 58254, 58254,
      58254, 58254, 58254, 58254, 55188, 58254, 58254, 55188,
      55188, 55188, 55188, 55188, 55188, 24966, 27594, 26214,
      26214, 26214, 27594, 24966, 23831, 24385, 25575, 25575,
      25575, 25575, 24385, 23831, 23302, 23302, 24966, 24966,
      24966, 23302, 23302, 21845, 22795, 24385, 24385, 22795,
      21845, 21400, 21845, 23831, 21845, 21400, 10382, 10700,
      10700, 10382, 10082, 9620, 10082, 9039, 9039, 8525, },
    { 8192, 65536, 65536, 61681, 61681, 61681, 41943, 41943,
      41943, 41943, 40330, 41943, 40330, 41943, 40330, 40330,
      40330, 38836, 38836, 40330, 40330, 24966, 27594, 26214,
      26214, 26214, 27594, 24966, 23831, 24385, 25575, 25575,
      25575, 25575, 24385, 23831, 11523, 11523, 12483, 12483,
      12483, 11523, 11523, 10923, 11275, 12193, 12193, 11275,
      10923, 5323, 5490, 5924, 5490, 5323, 5165, 5323,
      5323, 5165, 5017, 4788, 5017, 4520, 4520, 4263, }
};

static const int dv_weight_720[2][64] = {
    { 8192, 65536, 65536, 61681, 61681, 61681, 58254, 58254,
      58254, 58254, 58254, 58254, 55188, 58254, 58254, 55188,
      55188, 55188, 55188, 55188, 55188, 24966, 27594, 26214,
      26214, 26214, 27594, 24966, 23831, 24385, 25575, 25575,
      25575, 25575, 24385, 23831, 15420, 15420, 16644, 16644,
      16644, 15420, 15420, 10923, 11398, 12193, 12193, 11398,
      10923, 10700, 10923, 11916, 10923, 10700, 5191, 5350,
      5350, 5191, 5041, 4810, 5041, 4520, 4520, 4263, },
    { 8192, 43691, 43691, 40330, 40330, 40330, 29127, 29127,
      29127, 29127, 29127, 29127, 27594, 29127, 29127, 27594,
      27594, 27594, 27594, 27594, 27594, 12483, 13797, 13107,
      13107, 13107, 13797, 12483, 11916, 12193, 12788, 12788,
      12788, 12788, 12193, 11916, 5761, 5761, 6242, 6242,
      6242, 5761, 5761, 5461, 5638, 5461, 6096, 5638,
      5461, 2661, 2745, 2962, 2745, 2661, 2583, 2661,
      2661, 2583, 2509, 2394, 2509, 2260, 2260, 2131, }
};

/**
 * The "inverse" DV100 weights are actually just the spec weights (zig-zagged).
 */
static const int dv_iweight_1080_y[64] = {
    128,  16,  16,  17,  17,  17,  18,  18,
     18,  18,  18,  18,  19,  18,  18,  19,
     19,  19,  19,  19,  19,  42,  38,  40,
     40,  40,  38,  42,  44,  43,  41,  41,
     41,  41,  43,  44,  45,  45,  42,  42,
     42,  45,  45,  48,  46,  43,  43,  46,
     48,  49,  48,  44,  48,  49, 101,  98,
     98, 101, 104, 109, 104, 116, 116, 123,
};
static const int dv_iweight_1080_c[64] = {
    128,  16,  16,  17,  17,  17,  25,  25,
     25,  25,  26,  25,  26,  25,  26,  26,
     26,  27,  27,  26,  26,  42,  38,  40,
     40,  40,  38,  42,  44,  43,  41,  41,
     41,  41,  43,  44,  91,  91,  84,  84,
     84,  91,  91,  96,  93,  86,  86,  93,
     96, 197, 191, 177, 191, 197, 203, 197,
    197, 203, 209, 219, 209, 232, 232, 246,
};
static const int dv_iweight_720_y[64] = {
    128,  16,  16,  17,  17,  17,  18,  18,
     18,  18,  18,  18,  19,  18,  18,  19,
     19,  19,  19,  19,  19,  42,  38,  40,
     40,  40,  38,  42,  44,  43,  41,  41,
     41,  41,  43,  44,  68,  68,  63,  63,
     63,  68,  68,  96,  92,  86,  86,  92,
     96,  98,  96,  88,  96,  98, 202, 196,
    196, 202, 208, 218, 208, 232, 232, 246,
};
static const int dv_iweight_720_c[64] = {
    128,  24,  24,  26,  26,  26,  36,  36,
     36,  36,  36,  36,  38,  36,  36,  38,
     38,  38,  38,  38,  38,  84,  76,  80,
     80,  80,  76,  84,  88,  86,  82,  82,
     82,  82,  86,  88, 182, 182, 168, 168,
    168, 182, 182, 192, 186, 192, 172, 186,
    192, 394, 382, 354, 382, 394, 406, 394,
    394, 406, 418, 438, 418, 464, 464, 492,
};

static const uint8_t dv_audio_shuffle525[10][9] = {
  {  0, 30, 60, 20, 50, 80, 10, 40, 70 }, /* 1st channel */
  {  6, 36, 66, 26, 56, 86, 16, 46, 76 },
  { 12, 42, 72,  2, 32, 62, 22, 52, 82 },
  { 18, 48, 78,  8, 38, 68, 28, 58, 88 },
  { 24, 54, 84, 14, 44, 74,  4, 34, 64 },

  {  1, 31, 61, 21, 51, 81, 11, 41, 71 }, /* 2nd channel */
  {  7, 37, 67, 27, 57, 87, 17, 47, 77 },
  { 13, 43, 73,  3, 33, 63, 23, 53, 83 },
  { 19, 49, 79,  9, 39, 69, 29, 59, 89 },
  { 25, 55, 85, 15, 45, 75,  5, 35, 65 },
};

static const uint8_t dv_audio_shuffle625[12][9] = {
  {   0,  36,  72,  26,  62,  98,  16,  52,  88}, /* 1st channel */
  {   6,  42,  78,  32,  68, 104,  22,  58,  94},
  {  12,  48,  84,   2,  38,  74,  28,  64, 100},
  {  18,  54,  90,   8,  44,  80,  34,  70, 106},
  {  24,  60,  96,  14,  50,  86,   4,  40,  76},
  {  30,  66, 102,  20,  56,  92,  10,  46,  82},

  {   1,  37,  73,  27,  63,  99,  17,  53,  89}, /* 2nd channel */
  {   7,  43,  79,  33,  69, 105,  23,  59,  95},
  {  13,  49,  85,   3,  39,  75,  29,  65, 101},
  {  19,  55,  91,   9,  45,  81,  35,  71, 107},
  {  25,  61,  97,  15,  51,  87,   5,  41,  77},
  {  31,  67, 103,  21,  57,  93,  11,  47,  83},
};

static const av_unused int dv_audio_frequency[3] = {
    48000, 44100, 32000,
};

/* macroblock bit budgets */
static const uint8_t block_sizes_dv2550[8] = {
    112, 112, 112, 112, 80, 80, 0, 0,
};

static const uint8_t block_sizes_dv100[8] = {
    80, 80, 80, 80, 80, 80, 64, 64,
};

enum dv_section_type {
     dv_sect_header  = 0x1f,
     dv_sect_subcode = 0x3f,
     dv_sect_vaux    = 0x56,
     dv_sect_audio   = 0x76,
     dv_sect_video   = 0x96,
};

enum dv_pack_type {
     dv_header525     = 0x3f, /* see dv_write_pack for important details on */
     dv_header625     = 0xbf, /* these two packs */
     dv_timecode      = 0x13,
     dv_audio_source  = 0x50,
     dv_audio_control = 0x51,
     dv_audio_recdate = 0x52,
     dv_audio_rectime = 0x53,
     dv_video_source  = 0x60,
     dv_video_control = 0x61,
     dv_video_recdate = 0x62,
     dv_video_rectime = 0x63,
     dv_unknown_pack  = 0xff,
};

#define DV_PROFILE_IS_HD(p) ((p)->video_stype & 0x10)
#define DV_PROFILE_IS_1080i50(p) (((p)->video_stype == 0x14) && ((p)->dsf == 1))
#define DV_PROFILE_IS_1080i60(p) (((p)->video_stype == 0x14) && ((p)->dsf == 0))
#define DV_PROFILE_IS_720p50(p)  (((p)->video_stype == 0x18) && ((p)->dsf == 1))

/* minimum number of bytes to read from a DV stream in order to
   determine the profile */
#define DV_PROFILE_BYTES (6*80) /* 6 DIF blocks */

/**
 * largest possible DV frame, in bytes (1080i50)
 */
#define DV_MAX_FRAME_SIZE 576000

/**
 * maximum number of blocks per macroblock in any DV format
 */
#define DV_MAX_BPM 8

const DVprofile* ff_dv_frame_profile(const DVprofile *sys,
                                  const uint8_t* frame, unsigned buf_size);
const DVprofile* ff_dv_codec_profile(AVCodecContext* codec);

static inline int dv_write_dif_id(enum dv_section_type t, uint8_t chan_num,
                                  uint8_t seq_num, uint8_t dif_num,
                                  uint8_t* buf)
{
    int fsc = chan_num & 1;
    int fsp = 1 - (chan_num >> 1);

    buf[0] = (uint8_t)t;       /* Section type */
    buf[1] = (seq_num  << 4) | /* DIF seq number 0-9 for 525/60; 0-11 for 625/50 */
             (fsc << 3) |      /* FSC: for 50 and 100Mb/s 0 - first channel; 1 - second */
             (fsp << 2) |      /* FSP: for 100Mb/s 1 - channels 0-1; 0 - channels 2-3 */
             3;                /* reserved -- always 1 */
    buf[2] = dif_num;          /* DIF block number Video: 0-134, Audio: 0-8 */
    return 3;
}


static inline int dv_write_ssyb_id(uint8_t syb_num, uint8_t fr, uint8_t* buf)
{
    if (syb_num == 0 || syb_num == 6) {
        buf[0] = (fr << 7) | /* FR ID 1 - first half of each channel; 0 - second */
                 (0  << 4) | /* AP3 (Subcode application ID) */
                 0x0f;       /* reserved -- always 1 */
    }
    else if (syb_num == 11) {
        buf[0] = (fr << 7) | /* FR ID 1 - first half of each channel; 0 - second */
                 0x7f;       /* reserved -- always 1 */
    }
    else {
        buf[0] = (fr << 7) | /* FR ID 1 - first half of each channel; 0 - second */
                 (0  << 4) | /* APT (Track application ID) */
                 0x0f;       /* reserved -- always 1 */
    }
    buf[1] = 0xf0 |            /* reserved -- always 1 */
             (syb_num & 0x0f); /* SSYB number 0 - 11   */
    buf[2] = 0xff;             /* reserved -- always 1 */
    return 3;
}

static inline void dv_calculate_mb_xy(DVVideoContext *s, DVwork_chunk *work_chunk, int m, int *mb_x, int *mb_y)
{
     *mb_x = work_chunk->mb_coordinates[m] & 0xff;
     *mb_y = work_chunk->mb_coordinates[m] >> 8;

     /* We work with 720p frames split in half. The odd half-frame (chan==2,3) is displaced :-( */
     if (s->sys->height == 720 && !(s->buf[1]&0x0C)) {
         *mb_y -= (*mb_y>17)?18:-72; /* shifting the Y coordinate down by 72/2 macro blocks */
     }
}

static inline int dv_work_pool_size(const DVprofile *d)
{
    int size = d->n_difchan*d->difseg_size*27;
    if (DV_PROFILE_IS_1080i50(d))
        size -= 3*27;
    if (DV_PROFILE_IS_720p50(d))
        size -= 4*27;
    return size;
}

#define TEX_VLC_BITS 9

void ff_dv_init_vlc(RL_VLC_ELEM dv_rl_vlc[1184]);

#if CONFIG_SMALL
#define DV_VLC_MAP_RUN_SIZE 15
#define DV_VLC_MAP_LEV_SIZE 23
#else
#define DV_VLC_MAP_RUN_SIZE  64
#define DV_VLC_MAP_LEV_SIZE 512 //FIXME sign was removed so this should be /2 but needs check
#endif

/* VLC encoding lookup table */
typedef struct dv_vlc_pair {
   uint32_t vlc;
   uint32_t size;
} dv_vlc_pair;

extern dv_vlc_pair dv_vlc_map[DV_VLC_MAP_RUN_SIZE][DV_VLC_MAP_LEV_SIZE];

#if CONFIG_SMALL
/* Converts run and level (where level != 0) pair into VLC, returning bit size */
static av_always_inline int dv_rl2vlc(int run, int level, int sign, uint32_t* vlc)
{
    int size;
    if (run < DV_VLC_MAP_RUN_SIZE && level < DV_VLC_MAP_LEV_SIZE) {
        *vlc = dv_vlc_map[run][level].vlc | sign;
        size = dv_vlc_map[run][level].size;
    } else {
        if (level < DV_VLC_MAP_LEV_SIZE) {
            *vlc = dv_vlc_map[0][level].vlc | sign;
            size = dv_vlc_map[0][level].size;
        } else {
            *vlc = 0xfe00 | (level << 1) | sign;
            size = 16;
        }
        if (run) {
            *vlc |= ((run < 16) ? dv_vlc_map[run-1][0].vlc :
                                  (0x1f80 | (run - 1))) << size;
            size +=  (run < 16) ? dv_vlc_map[run-1][0].size : 13;
        }
    }

    return size;
}

static av_always_inline int dv_rl2vlc_size(int run, int level)
{
    int size;

    if (run < DV_VLC_MAP_RUN_SIZE && level < DV_VLC_MAP_LEV_SIZE) {
        size = dv_vlc_map[run][level].size;
    } else {
        size = (level < DV_VLC_MAP_LEV_SIZE) ? dv_vlc_map[0][level].size : 16;
        if (run)
            size += (run < 16) ? dv_vlc_map[run-1][0].size : 13;
    }
    return size;
}
#else
static av_always_inline int dv_rl2vlc(int run, int l, int sign, uint32_t* vlc)
{
    *vlc = dv_vlc_map[run][l].vlc | sign;
    return dv_vlc_map[run][l].size;
}

static av_always_inline int dv_rl2vlc_size(int run, int l)
{
    return dv_vlc_map[run][l].size;
}
#endif

int ff_dv_init_dynamic_tables(const DVprofile *d);
void ff_dv_vlc_map_tableinit(void);

#endif /* AVCODEC_DVDATA_H */
