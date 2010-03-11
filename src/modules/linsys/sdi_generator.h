/**
 * sdi_generator.h
 **/

#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <pthread.h>

#include <signal.h>
#include <math.h>

#ifndef SDI_GENERATOR_H_
#define SDI_GENERATOR_H_

// defines only for SD NTSC (mkline funktion for test pattern)
#define VERT_BLANKING 0
#define MAIN_SET 1
#define CHROMA_SET 2
#define BLACK_SET 3
#define BLACK 4

// defines for SD SDI with blanking
#define ANCILLARY_DATA_SAMPLES 280
#define FIELD_1 1
#define FIELD_2 2
#define VERT_BLANKING 0
#define ACTIVE_VIDEO 1

// Master SDI device
#define SDI_IOC_MAGIC '='
#define SDI_IOC_TXGETEVENTS	_IOR(SDI_IOC_MAGIC, 2, unsigned int)

// Transmitter event flag bit locations
#define SDI_EVENT_TX_BUFFER_ORDER	0
#define SDI_EVENT_TX_BUFFER		(1 << SDI_EVENT_TX_BUFFER_ORDER)
#define SDI_EVENT_TX_FIFO_ORDER		1
#define SDI_EVENT_TX_FIFO		(1 << SDI_EVENT_TX_FIFO_ORDER)
#define SDI_EVENT_TX_DATA_ORDER		2
#define SDI_EVENT_TX_DATA		(1 << SDI_EVENT_TX_DATA_ORDER)


// part of the linsys sdiaudio.h

#define SDIAUDIO_IOC_TXGETCAP			_IOR(SDIAUDIO_IOC_MAGIC, 1, unsigned int)
#define SDIAUDIO_IOC_TXGETEVENTS		_IOR(SDIAUDIO_IOC_MAGIC, 2, unsigned int)
#define SDIAUDIO_IOC_TXGETBUFLEVEL		_IOR(SDIAUDIO_IOC_MAGIC, 3, unsigned int)
#define SDIAUDIO_IOC_TXGETTXD			_IOR(SDIAUDIO_IOC_MAGIC, 4, int)

#define SDIAUDIO_IOC_MAGIC '~' /* This ioctl magic number is currently free. See
			   * /usr/src/linux/Documentation/ioctl-number.txt */
/* Transmitter event flag bit locations */
#define SDIAUDIO_EVENT_TX_BUFFER_ORDER	0
#define SDIAUDIO_EVENT_TX_BUFFER	(1 << SDIAUDIO_EVENT_TX_BUFFER_ORDER)
#define SDIAUDIO_EVENT_TX_FIFO_ORDER	1
#define SDIAUDIO_EVENT_TX_FIFO		(1 << SDIAUDIO_EVENT_TX_FIFO_ORDER)
#define SDIAUDIO_EVENT_TX_DATA_ORDER	2
#define SDIAUDIO_EVENT_TX_DATA		(1 << SDIAUDIO_EVENT_TX_DATA_ORDER)

// Filehandler for sdi output
static int fh_sdi_video;
static int fh_sdi_audio;


#define MAX_SAMPLES_PER_LINE (2*2750)
#define MAX_LINES_PER_FRAME 1125
#define MAX_AUDIO_STREAMS (8)
// max. audio samples per frame
#define MAX_AUDIO_SAMPLES (2002*2)
/**
 * 23.98Hz = fix:{2002}
 * 24Hz = fix:{2000}
 * 25Hz = fix:{1920}
 * 29.97Hz = vari:{1601,1602,1602}
 * 30Hz = fix:{1600}
 **/

#define MAX_SDI_HEIGHT 1125			// HD-SDI
#define MAX_SDI_WIDTH 2750			// HD-SDI (FMT_1080p24 has up to 2750)
#define MAX_SDI_FRAMESIZE (MAX_SDI_HEIGHT*MAX_SDI_WIDTH*2) // SDI frame size, (2 Pixels are represented by 4 bytes, yuyv422)
struct source_format {
	unsigned int lines_per_frame;
	unsigned int active_lines_per_frame;
	unsigned int samples_per_line;
	unsigned int active_samples_per_line;
	unsigned int interlaced;
};

struct audio_format {

	mlt_audio_format aformat; // default: mlt_audio_pcm
	uint16_t samples; // default 2*1920
	uint16_t sample_rate; // default 48000
	int channels; // default 2 (stereo)
/**
 * 0 channels = audio disabled, transmit only
 * 2 channels (stereo)
 * 4 channels
 * 6 channels
 * 8 channels
 **/
};

// HD
static const struct source_format FMT_1080i60 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2200,
		.active_samples_per_line = 2*1920, .interlaced = 1 };

static const struct source_format FMT_1080i5994 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2200,
		.active_samples_per_line = 2*1920, .interlaced = 1 };

static const struct source_format FMT_1080i50 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2640,
		.active_samples_per_line = 2*1920, .interlaced = 1 };

static const struct source_format FMT_1080p30 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2200,
		.active_samples_per_line = 2*1920, .interlaced = 0 };

static const struct source_format FMT_1080p2997 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2200,
		.active_samples_per_line = 2*1920, .interlaced = 0 };

static const struct source_format FMT_1080p25 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2640,
		.active_samples_per_line = 2*1920, .interlaced = 0 };

static const struct source_format FMT_1080p24 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2750,
		.active_samples_per_line = 2*1920, .interlaced = 0 };

static const struct source_format FMT_1080p2398 = { .lines_per_frame = 1125, .active_lines_per_frame = 1080, .samples_per_line = 2*2750,
		.active_samples_per_line = 2*1920, .interlaced = 0 };

static const struct source_format FMT_720p60 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*1650,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p5994 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*1650,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p50 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*1980,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p30 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*3300,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p2997 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*3300,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p25 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*3960,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p24 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*4125,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

static const struct source_format FMT_720p2398 = { .lines_per_frame = 750, .active_lines_per_frame = 720, .samples_per_line = 2*4125,
		.active_samples_per_line = 2*1280, .interlaced = 0 };

// SD PAL
static const struct source_format FMT_576i50 = { .lines_per_frame = 625, .active_lines_per_frame = 576, .samples_per_line = 2*864 /*1728*/,
		.active_samples_per_line = 2*720 /* 720xY, 360xCb, 360xCr */, .interlaced = 1 };

// SD NTSC
static const struct source_format FMT_480i5994 = { .lines_per_frame = 525, .active_lines_per_frame = 486, .samples_per_line = 2*858 /*1716*/,
		.active_samples_per_line = 2*720 /* 720xY, 360xCb, 360xCr */, .interlaced = 1 };

struct trs {
	unsigned short int sav;
	unsigned short int eav;
};

static const struct trs FIELD_1_ACTIVE = { .sav = 0x200, .eav = 0x274 };
static const struct trs FIELD_1_VERT_BLANKING = { .sav = 0x2ac, .eav = 0x2d8 };
static const struct trs FIELD_2_ACTIVE = { .sav = 0x31c, .eav = 0x368 };
static const struct trs FIELD_2_VERT_BLANKING = { .sav = 0x3b0, .eav = 0x3c4 };

struct line_info {
	const struct source_format *fmt;
	unsigned int ln;
	const struct trs *xyz;
	uint8_t blanking;
};

struct SDI_atr {
	int status;
	int *fh;
	uint8_t *data;
	size_t framesize;
} SDI_atr;

// 192bit for AESChannelStatusBits
uint8_t AESChannelStatusBitArray[192]; // beta array
//uint8_t AESChannelStatusBitArray[24]; // TODO better way for 24x8bit !!!

// buffer for one sdi line
uint16_t * line_buffer;
// counter for active line number
uint16_t active_video_line;
// buffer for sdi frame size
uint64_t sdi_frame_size;
// buffer for the complete SDI frame
uint8_t * data;

static char * device_file_video;
static char * device_file_audio;
static struct line_info info;
static uint8_t *(*pack)(uint8_t *outbuf, unsigned short int *inbuf, size_t count);
static size_t elements;
static unsigned int samples;

// functions
static int sdi_init(char *device_video, char *device_audio, uint8_t blanking, mlt_profile myProfile);

static int sdimaster_close();

static int sdi_playout(uint8_t *vBuffer, int16_t aBuffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES], const struct audio_format * audio_format, int audio_streams,
		int my_DBN);

static int mkline(unsigned short int *buf, const struct line_info *info, unsigned int pattern);

static inline int create_HD_SDI_Line(uint16_t *buf, const struct line_info *info, uint16_t active_video_line, unsigned int active, uint8_t *video_buffer);
static inline int create_SD_SDI_Line(uint16_t *buf, const struct line_info *info, int field, int active, uint8_t *video_buffer,
		int16_t audio_buffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES], int linenumber_sdiframe, int active_video_line, int my_DBN, int16_t AudioGroupCounter,
		int16_t AudioGroups2Write, int audio_streams);

static int writeANC(uint16_t *p, int linenumber_sdiframe, uint16_t DID, int my_DBN, int16_t *audio_buffer_A, int16_t *audio_buffer_B,
		int16_t AudioDataPacketCounter, int16_t AudioGroups2Write);
static uint16_t checker(uint16_t *DID_pointer);

static uint8_t getZBit(int sample_number);
static uint8_t getChannelStatusBit(uint16_t sample_number, uint8_t ch);
static int16_t getNumberOfAudioGroups2Write(int linenuber);

static uint8_t getDBN(int my_DBN);

static inline uint8_t *pack8(uint8_t *outbuf, uint16_t *inbuf, size_t count); // alias 'pack_uyvy()'
static inline uint8_t *pack10(uint8_t *outbuf, uint16_t *inbuf, size_t count);
static inline uint8_t *pack_v210(uint8_t *outbuf, uint16_t *inbuf, size_t count);

static int pack_AES_subframe(uint16_t *p, int8_t c, int8_t z, int8_t ch, int16_t *audio_sample);

#endif /* SDI_GENERATOR_H_ */
