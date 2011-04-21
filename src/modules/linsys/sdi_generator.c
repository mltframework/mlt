/**
 *
 * MLT SDI Consumer:
 * request video and audio data from MLT and generate an SDI stream
 *
 * Copyright (C) Broadcasting Center Europe S.A. http://www.bce.lu
 * an RTL Group Company  http://www.rtlgroup.com
 * All rights reserved.
 *
 * E-mail: support_plasec@bce.lu
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * DESCRIPTION:
 * This software act as interface between the MLT Frameworkas as
 * MLT Consumer and the Linear Systems Ltd. SMPTE 292M and SMPTE 259M-C boards.
 *
 * Linear Systems can be contacted at http://www.linsys.ca
 *
 **********************************************************************************
 *      System : INTeL I686 64Bit
 *          OS : Linux SuSE Kernel 2.6.27.39-0.2-default
 *     Compiler: gcc 4.3.2 (c++)
 **********************************************************************************
 *     Project : MLT SDI Consumer for SD and HD
 *  Started by : Thomas Kurpick, Dipl.Inf. (FH)
 **********************************************************************************
 *  Supported and tested boards for SD-SDI or HD-SDI Output:
 *      PCI SDI Master™              (model 107)
 *  	PCIe SDI Master™             (model 159)
 *      PCIe LP SDI Master™ FD       (model 145)
 *      PCIe LP SDI Master™ Quad/o   (model 180)
 *      PCIe LP HD-SDI Master™ O     (model 193)
 *
 *  Note: PCIe LP HD-SDI Master™ O (model 193) is an VidPort model and supports an
 *  seperate video and audio interface. Device file:
 * 		/dev/sdivideotx[]	for active video data
 *  	/dev/sdiaudiotx[]	for pcm audio data
 *
 *	This mlt consumer use the following device files:
 *   /dev/sditx[]         (SD-PAL)	up to 8 x AES (8 x stereo / 16 audio channels)
 *   /dev/sdivideotx[]    (HD)
 *   /dev/sdiaudiotx[]    (HD)		up to 4 x AES (4 x stereo / 8 audio channels)
 *
 *
 **********************************************************************************
 * Last modified by:
 * Thomas Kurpick                     08.Jan.2010
 * Ver. 2.0
 *
 **********************************************************************************
 *
 * Consumer properties:
 * 	'dev_video'
 * 	'dev_audio'
 * 	'blanking'
 * Only to monitor the SDI output a beta version of jpeg-writer is implemented.
 * 	'jpeg_files' a number for output interval
 * 	'save_jpegs' path for image
 *
 * EXAMPLE:
 *
 * SDI boards with full frame stream (with blanking):
 * 	melt video.dv -consumer sdi:/dev/sditx0 buffer=0;
 * 	melt video.dv -consumer sdi:/dev/sditx0 buffer=0 blanking=true;
 * 	melt video.dv -consumer sdi dev_video=/dev/sditx0 buffer=0 blanking=true;
 * 	melt video.dv audio_index=all -consumer sdi dev_video=/dev/sditx0 buffer=0 blanking=true;
 *
 * SDI boards without full frame stream (without blanking):
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=1   -consumer sdi dev_video=/dev/sdivideotx0 dev_sdiaudio=/dev/sdiaudiotx0 blanking=false
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=all -consumer sdi dev_video=/dev/sdivideotx0 dev_sdiaudio=/dev/sdiaudiotx0 blanking=false
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=all -consumer sdi dev_video=/dev/sdivideotx0 dev_sdiaudio=/dev/sdiaudiotx0 blanking=false jpeg_files=25 save_jpegs=channel_04.jpg
 *
 *
 * SDI output formats and MLT profiles:
 * #####################################################################################################################################################
 * ########## SMPTE 274M 1920 x 1080 Image Sample Structure ############################################################################################
 * #####################################################################################################################################################
 * System No.	System nomenclature		Form of scanning	Frame rate				Embedded Audio			MLT profile		Linsys board support (model)
 * 	4			1920 x 1080/60/I		interlaced			30 HZ					4 x AES (8 channels)	atsc_1080i_60	193
 * 	5			1920 x 1080/59.94/I		interlaced			30000/1001 ~ 29.97 HZ	4 x AES (8 channels)	atsc_1080i_5994	193
 * 	6			1920 x 1080/50/I		interlaced			25 HZ					4 x AES (8 channels)	atsc_1080i_50	193
 * 	7			1920 x 1080/30/P		progressive			30 HZ					4 x AES (8 channels)	atsc_1080p_30	193
 * 	8			1920 x 1080/29.97/P		progressive			30000/1001 ~ 29.97 HZ	4 x AES (8 channels)	atsc_1080p_2997	193
 * 	9			1920 x 1080/25/P		progressive			25 HZ					4 x AES (8 channels)	atsc_1080p_25	193
 * 	10			1920 x 1080/24/P		progressive			24 HZ					4 x AES (8 channels)	atsc_1080p_24	193
 * 	11			1920 x 1080/23.98/P		progressive			24000/1001 ~ 23.98 HZ	4 x AES (8 channels)	atsc_1080p_2398	193
 *
 * #####################################################################################################################################################
 * ########## SMPTE 296M 1280 × 720 Progressive Image Sample Structure #################################################################################
 * #####################################################################################################################################################
 * System No.	System nomenclature		Form of scanning	Frame rate				Embedded Audio			MLT profile		Linsys board support (model)
 * 1				1280 × 720/60		progressive			60 HZ					4 x AES (8 channels)	atsc_720p_60	193
 * 2				1280 × 720/59.94	progressive			60000/1001 ~ 59.97 HZ	4 x AES (8 channels)	atsc_720p_5994	193
 * 3				1280 × 720/50		progressive			50 HZ					4 x AES (8 channels)	atsc_720p_50	193
 * 4				1280 × 720/30		progressive			30 HZ					4 x AES (8 channels)	atsc_720p_30	193
 * 5				1280 × 720/29.97	progressive			30000/1001 ~ 29.97 HZ	4 x AES (8 channels)	atsc_720p_2997	193
 * 6				1280 × 720/25		progressive			25 HZ					4 x AES (8 channels)	atsc_720p_25	193
 * 7				1280 × 720/24		progressive			24 HZ					4 x AES (8 channels)	atsc_720p_24	193
 * 8				1280 × 720/23.98	progressive			24000/1001 ~ 23.98 HZ	4 x AES (8 channels)	atsc_720p_2398	193
 *
 * #####################################################################################################################################################
 * ########## SMPTE 125M 486i 29.97Hz & BT.656 576i 25Hz ###############################################################################################
 * #####################################################################################################################################################
 * System No.	System nomenclature		Form of scanning	Frame rate				Embedded Audio			MLT profile		Linsys board support (model)
 * SD PAL		720 × 576/50/I			interlaced			25 HZ					8 x AES (16 channels)	dv_pal			180,145,159,107
 * SD PAL		720 × 576/50/I			interlaced			25 HZ					4 x AES (8 channels)	dv_pal			193
 * SD NTSC		720 × 480/59.94/I		interlaced			30000/1001 ~ 29.97 HZ	8 x AES (16 channels)	sdi_486i_5994	TODO:180,145,159,107
 * SD NTSC		720 × 480/59.94/I		interlaced			30000/1001 ~ 29.97 HZ	4 x AES (8 channels)	sdi_486i_5994	193
 *
 **/

#include "sdi_generator.h"

/*!/brief initialization of the file handlers for the playout
 * @param *device_video: file or SDITX device or SDIVIDEOTX device
 * @param *device_audio: file or SDIAUDIOTX device
 * @param blanking: true or false (if false the consumer write only active video data without any VANH or HANC)
 */
static int sdi_init(char *device_video, char *device_audio, uint8_t blanking, mlt_profile myProfile, const struct audio_format * audio_format) {

	// set device file
	device_file_video = device_video;
	device_file_audio = device_audio;

	// set flag for using of blanking with ancillary data
	info.blanking = blanking;

	// set pack method for SDI word conversion
	pack = pack8;
	//pack = pack10;
	//pack = pack_v210;

	// check format
	if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 30 && myProfile->frame_rate_den == 1 && myProfile->progressive
			== 0) {
		info.fmt = &FMT_1080i60;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080I_60HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 30000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 0) {
		info.fmt = &FMT_1080i5994;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080I_59_94HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 25 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 0) {
		info.fmt = &FMT_1080i50;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080I_50HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 30 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_1080p30;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080P_30HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 30000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_1080p2997;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080P_29_97HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 25 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_1080p25;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080P_25HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 24 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_1080p24;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080P_24HZ;
	} else if (myProfile->width == 1920 && myProfile->height == 1080 && myProfile->frame_rate_num == 24000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_1080p2398;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_274M_1080P_23_98HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 60 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p60;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_60HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 60000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p5994;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_59_94HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 50 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p50;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_50HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 30 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p30;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_30HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 30000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p2997;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_29_97HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 25 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p25;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_25HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 24 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p24;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_24HZ;
	} else if (myProfile->width == 1280 && myProfile->height == 720 && myProfile->frame_rate_num == 24000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 1) {
		info.fmt = &FMT_720p2398;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_296M_720P_23_98HZ;
	} else if (myProfile->width == 720 && myProfile->height == 576 && myProfile->frame_rate_num == 25 && myProfile->frame_rate_den == 1
			&& myProfile->progressive == 0) {
		info.fmt = &FMT_576i50;
		sdi_frame_mode = SDIVIDEO_CTL_BT_601_576I_50HZ;
	} else if (myProfile->width == 720 && myProfile->height == 480 && myProfile->frame_rate_num == 30000 && myProfile->frame_rate_den == 1001
			&& myProfile->progressive == 0) {
		info.fmt = &FMT_480i5994;
		sdi_frame_mode = SDIVIDEO_CTL_SMPTE_125M_486I_59_94HZ;
	} else {
		printf("Consumer got unknown format: %s", myProfile->description);
		info.fmt = &FMT_576i50;
		sdi_frame_mode = SDIVIDEO_CTL_BT_601_576I_50HZ;
	}

	printf("Consumer use format: %s\nProfile: %i %i %i %i %i\n", myProfile->description, myProfile->width, myProfile->height, myProfile->frame_rate_num,
			myProfile->frame_rate_den, myProfile->progressive);

	// Check if the format supports own blanking (note: model 193 supports currently only active video at the video device file)
	if (info.blanking && info.fmt != &FMT_576i50) {
		printf("SDI consumer doesn't support blanking(HANC) for the configured SD board and SDI format. Try argument: blanking=false\n");
		return EXIT_FAILURE;
	}

	// if we write our own HANC we need an AES channel status bit array
	if (info.blanking) {

		// small description
		// http://www.sencore.com/newsletter/Nov05/DigAudioChannelStatusBits.htm
		// or
		// http://www.sencore.com/uploads/files/DigAudioChannelStatusBits.pdf

		// create empty AESChannelStatusBitArray
		int i = 0;
		for (i = 0; i < sizeof(AESChannelStatusBitArray) / sizeof(AESChannelStatusBitArray[0]); i++)
			AESChannelStatusBitArray[i] = 0;

		/**
		 * Professional Format - Channel Status Bits
		 **/
		////// Byte 0 //////
		AESChannelStatusBitArray[0] = 1; // professional format

		AESChannelStatusBitArray[1] = 0; // PCM Format

		AESChannelStatusBitArray[2] = 1; // Emphasis: [100] No Emphasis
		AESChannelStatusBitArray[3] = 0; // ^
		AESChannelStatusBitArray[4] = 0; // ^

		AESChannelStatusBitArray[5] = 0; // locked

		AESChannelStatusBitArray[6] = 0; // sample frequency Fs: [01]48kHz, [10]44kHz, [11]32kHz
		AESChannelStatusBitArray[7] = 1; // ^
		////// Byte 1 //////
		AESChannelStatusBitArray[8] = 0; // channel mode: [0000] not indicated, [0001]2channels, [0010]1channel mono, ...
		AESChannelStatusBitArray[9] = 0; // ^
		AESChannelStatusBitArray[10] = 0; // ^
		AESChannelStatusBitArray[11] = 1; // ^
		////// Byte 2 //////
		AESChannelStatusBitArray[19] = 0; // Encoded sample word length [100]20bits,
		AESChannelStatusBitArray[20] = 0; //
		AESChannelStatusBitArray[21] = 0; //
		////// Byte 3 //////
		AESChannelStatusBitArray[24] = 0; //
		AESChannelStatusBitArray[25] = 0; //
		AESChannelStatusBitArray[26] = 0; //
		AESChannelStatusBitArray[27] = 0; //
		AESChannelStatusBitArray[28] = 0; //
		AESChannelStatusBitArray[29] = 0; //
		AESChannelStatusBitArray[30] = 0; //
		AESChannelStatusBitArray[31] = 0; // Multi Channel Mode
		////// Byte 4-21 //////
		//AESChannelStatusBitArray[32-179]= 0;
		////// Byte 22 //////
		AESChannelStatusBitArray[180] = 0; // Reliability Flags
		AESChannelStatusBitArray[181] = 1; // ^
		AESChannelStatusBitArray[182] = 1; // ^
		AESChannelStatusBitArray[183] = 1; // ^
		////// Byte 23 //////
		AESChannelStatusBitArray[184] = 0; // Cyclic Redundancy Check
		AESChannelStatusBitArray[185] = 1; // ^
		AESChannelStatusBitArray[186] = 0; // ^
		AESChannelStatusBitArray[187] = 0; // ^
		AESChannelStatusBitArray[188] = 1; // ^
		AESChannelStatusBitArray[189] = 0; // ^
		AESChannelStatusBitArray[190] = 1; // ^
		AESChannelStatusBitArray[191] = 1; // ^
	}

	// set buffer for one line of active video samples
	line_buffer = (uint16_t*) calloc(info.fmt->samples_per_line, sizeof(uint16_t));

	// calculate and set buffer for the complete SDI frame
	if (info.fmt != &FMT_576i50 && info.fmt != &FMT_480i5994) {
		if (info.blanking) {
			if (pack == pack_v210) {
				samples = (info.fmt->samples_per_line / 96 * 48) + ((info.fmt->samples_per_line % 96) ? 48 : 0);
				sdi_frame_size = samples * info.fmt->lines_per_frame * 8 / 3;
			} else {
				sdi_frame_size = info.fmt->samples_per_line * info.fmt->lines_per_frame;
			}
		} else {
			if (pack == pack_v210) {
				samples = (info.fmt->active_samples_per_line / 96 * 48) + ((info.fmt->active_samples_per_line % 96) ? 48 : 0);
				sdi_frame_size = samples * info.fmt->active_lines_per_frame * 8 / 3;
			} else {
				sdi_frame_size = info.fmt->active_samples_per_line * info.fmt->active_lines_per_frame;
			}
		}
	} else {
		if (info.blanking) {
			if (pack == pack_v210) {
				sdi_frame_size = info.fmt->samples_per_line * 4 / 3 * info.fmt->lines_per_frame;
			} else if (pack == pack8) {
				sdi_frame_size = info.fmt->samples_per_line * info.fmt->lines_per_frame;
			} else {
				sdi_frame_size = info.fmt->samples_per_line * 10 / 8 * info.fmt->lines_per_frame;
			}
		} else {
			if (pack == pack_v210) {
				sdi_frame_size = info.fmt->active_samples_per_line * 4 / 3 * info.fmt->active_lines_per_frame;
			} else if (pack == pack8) {
				sdi_frame_size = info.fmt->active_samples_per_line * info.fmt->active_lines_per_frame;
			} else {
				sdi_frame_size = info.fmt->active_samples_per_line * 10 / 8 * info.fmt->active_lines_per_frame;
			}
		}
	}

	// (*10/8 because we store (TOTAL_SAMPLES*TOTAL_LINES) words with 10 bit in this 8 bit array) )
	if (info.fmt == &FMT_576i50 && info.blanking) {
		sdi_frame_size = info.fmt->samples_per_line * 10 / 8 * info.fmt->lines_per_frame;
	}

	if (info.blanking) {
		printf("SDI frame size: %li\n", sdi_frame_size);
	} else {
		printf("Frame size for active video: %li\n", sdi_frame_size);
	}

	/**
	 * Setup HD-SDI Master device (vidport):
	 *
	 * if device_file_video available then
	 * 	if vidport available
	 * 		1. setup
	 * 	end
	 * 	1. open device file handler
	 *
	 * 	if device_file_audio available then
	 * 		1. setup
	 * 		2. open device file handler
	 * 	end
	 * end
	 **/
	if (device_file_video != NULL) {

		// If we use a Linsys HD board with active video (without blanking) setup the board for the used mode
		if (strstr(device_file_video, "sdivideotx") != NULL && !info.blanking) {

			char * value;

			// Buffer size
			value = itoa(sdi_frame_size);
			setSDIVideoProperties(SETTING_BUFFER_SIZE_VIDEO, value, device_video);
			free(value);

			// Frame Mode
			value = itoa(sdi_frame_mode);
			setSDIVideoProperties(SETTING_FRAME_MODE, value, device_video);
			free(value);

			// Data Mode
			if (pack == pack8)
				setSDIVideoProperties(SETTING_DATA_MODE, "0", device_video);
			else if (pack == pack_v210)
				setSDIVideoProperties(SETTING_DATA_MODE, "1", device_video);
		}

		// open file handle for SDI(video) output
		if ((fh_sdi_video = open(device_file_video, O_WRONLY)) == -1) {
			perror(NULL);
			printf("\ncould not open video output destination: %s\n", device_file_video);
			return EXIT_FAILURE;
		}
		printf("SDI consumer uses video device file: %s\n", device_file_video);

		// Check if we have to use a separate device file for audio
		if (device_file_audio != NULL) {

			// set settings for audio device file
			if (strstr(device_file_audio, "sdiaudiotx") != NULL && !info.blanking) {

				char * value;

				/**
				 * prepare sample size
				 * MLT suports: 16bit, 32bit
				 * LINSYS SDI boards supports: 16bit, 24bit, 32bit
				 * we set 16bit as default
				 **/
				uint8_t sample_size = audio_format->aformat == mlt_audio_s32 ? 32 : 16;

				// Buffer size
				// audio buffer per frame (Bytes) = sample rate / frame rate * ( sample size / 1Byte ) x channels
				value = itoa(audio_format->sample_rate / (myProfile->frame_rate_num / myProfile->frame_rate_den) * sample_size / 8 * audio_format->channels );
				setSDIAudioProperties(SETTING_BUFFER_SIZE_AUDIO, value, device_audio);
				free(value);

				// channels
				value = itoa(audio_format->channels);
				setSDIAudioProperties(SETTING_CHANNELS, value, device_audio);
				free(value);

				// sample rate
				value = itoa(audio_format->sample_rate);
				setSDIAudioProperties(SETTING_SAMPEL_RATE, value, device_audio);
				free(value);

				// sample size
				value = itoa(sample_size);
				setSDIAudioProperties(SETTING_SAMPLE_SIZE, value, device_audio);
				free(value);
			}

			// open file handle for audio output
			if ((fh_sdi_audio = open(device_file_audio, O_WRONLY)) == -1) {
				perror(NULL);
				printf("\nCould not open audio output destination: %s\n", device_file_audio);
				return EXIT_FAILURE;
			}
			printf("SDI consumer uses audio device file: %s\n", device_file_audio);
		}

	}

	// set buffer for the complete SDI frame
	data = (uint8_t*) calloc(sdi_frame_size, sizeof(uint8_t));

	return 1;
}

/**
 * Writes video and audio to specified files in SDI format
 * @param *vBuffer: Pointer to a video Buffer
 * @param aBuffer[][]
 * @param *audio_format: mlt audio_format
 * @param audio_streams: number of audio streams which have content in aBuffer (available 0-8)
 * @return current DBN (data block number of SDI frame)
 **/
static int sdi_playout(uint8_t *vBuffer, int16_t aBuffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES], const struct audio_format * audio_format, int audio_streams,
		int my_DBN) {

	// Pointer to the start of data. This is used to fill data line by line
	uint8_t *p = data;

	//*******************************************************************************************
	//****************	Build the SDI frame line by line ****************************************
	//*******************************************************************************************

	/*
	 * if SDI FMT_576i50 for card ASSY 145 or ASSY 159, with access to whole SDI frame buffer
	 *   and device_file_audio must be NULL
	 *   	than we write own audio data,
	 * else
	 *   	than HD for card ASSY 193
	 */
	//if (info.fmt == &FMT_576i50 && device_file_audio == NULL && !strcmp(device_file_video, "/dev/sdivideotx0")) {
	if (info.fmt == &FMT_576i50 && info.blanking) {

		//counter for the lines
		int i = 0;
		int16_t AudioGroupCounter = 0;

		/*#####################################################*/
		/*########	FIELD 1				#######################*/
		/*#####################################################*/

		info.xyz = &FIELD_1_VERT_BLANKING;

		// line 1-22	VERTICAL_BLANKING:23 lines				SAV 0x2ac		EAV 0x2d8
		for (i = 1; i <= 5; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}
		for (i = 6; i <= 8; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}
		for (i = 9; i <= 22; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}
		// line 23-310  ACTIVE: 287 lines						SAV 0x200		EAV 0x274
		info.xyz = &FIELD_1_ACTIVE;
		int f1counter = 1; // only odd lines
		for (i = 23; i <= 310; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_1, ACTIVE_VIDEO, vBuffer, aBuffer, i, f1counter, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
			f1counter += 2;
		}
		i = 311;
		// line 311-312 VERTICAL_BLANKING: 2 lines				SAV 0x2ac		EAV 0x2d8
		info.xyz = &FIELD_1_VERT_BLANKING;
		create_SD_SDI_Line(line_buffer, &info, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);
		create_SD_SDI_Line(line_buffer, &info, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		/*#####################################################*/
		/*########      FIELD 2        ########################*/
		/*#####################################################*/

		info.xyz = &FIELD_2_VERT_BLANKING;

		// line 313-336 VERTICAL_BLANKING: 23 lines				SAV 0x3b0		EAV 0x3c4
		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
				getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
		p = pack10(p, line_buffer, info.fmt->samples_per_line);

		// `getAudioGroups2Write()`=0
		for (i = 319; i <= 321; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}
		for (i = 322; i <= 335; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}
		// line 336-623 ACTIVE: 288 lines						SAV 0x31c		EAV 0x368
		info.xyz = &FIELD_2_ACTIVE;
		int f2counter = 2; // only even Lines
		for (i = 336; i <= 623; i++) {

			create_SD_SDI_Line(line_buffer, &info, FIELD_2, ACTIVE_VIDEO, vBuffer, aBuffer, i, f2counter, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
			f2counter += 2;
		}
		// line 624-625 VERTICAL_BLANKING: 2 lines				SAV 0x3b0		EAV 0x3c4
		info.xyz = &FIELD_2_VERT_BLANKING;
		for (i = 624; i <= 625; i++) {
			create_SD_SDI_Line(line_buffer, &info, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0, getDBN(my_DBN++), AudioGroupCounter,
					getNumberOfAudioGroups2Write(i), audio_streams);
			AudioGroupCounter += getNumberOfAudioGroups2Write(i);
			p = pack10(p, line_buffer, info.fmt->samples_per_line);
		}

	} else { // use HD board without blanking

		// start with first even line
		active_video_line = 1;

		/* *****************************************
		 * *********** LINE DISTRIBUTION ***********
		 * *****************************************
		 *
		 * << decide form of scanning (interlaced || progressive) >>
		 * if (interlaced)
		 * 		<< decide lines per frame (1125 || 625 || 525) >>
		 * 		if(1125)					1080x1920 HD
		 *			than create lines
		 * 		else if(625)				  576x720 PAL
		 *			than create lines
		 * 		else (525)					  486x720 NTSC
		 *			than create lines
		 * else (progressive)
		 * 		<< decide resolution (1125 || 750) >>
		 * 		if(1125)					1080x1920 HD
		 *			than create lines
		 * 		else(750)					 720x1280 HD
		 *			than create lines
		 *
		 **/

		// Generate a frame
		if (info.fmt->interlaced) {

			/****************************************
			 * INTERLACED
			 ****************************************/

			if (info.fmt->lines_per_frame == 1125) {

				if (info.blanking) {
					elements = info.fmt->samples_per_line;
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1; info.ln <= 20; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				} else {
					elements = info.fmt->active_samples_per_line;
				}
				info.xyz = &FIELD_1_ACTIVE;
				for (info.ln = 21; info.ln <= 560; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 561; info.ln <= 563; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
					info.xyz = &FIELD_2_VERT_BLANKING;
					for (info.ln = 564; info.ln <= 583; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
				// start with first odd line
				active_video_line = 2;

				info.xyz = &FIELD_2_ACTIVE;
				for (info.ln = 584; info.ln <= 1123; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_2_VERT_BLANKING;
					for (info.ln = 1124; info.ln <= 1125; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
			} else if (info.fmt->lines_per_frame == 625) {

				elements = info.fmt->active_samples_per_line;

				// start with first even line
				active_video_line = 1;

				/**
				 *  Generate an SDI PAL frame
				 **/
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1; info.ln <= 22; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
				info.xyz = &FIELD_1_ACTIVE;
				for (info.ln = 23; info.ln <= 310; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 311; info.ln <= 312; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
					info.xyz = &FIELD_2_VERT_BLANKING;
					for (info.ln = 313; info.ln <= 335; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}

				// start with first odd line
				active_video_line = 2;

				info.xyz = &FIELD_2_ACTIVE;
				for (info.ln = 336; info.ln <= 623; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_2_VERT_BLANKING;
					for (info.ln = 624; info.ln <= 625; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
			} else if (info.fmt->lines_per_frame == 525) {

				/**
				 *  Generate an SDI NTSC frame
				 **/
				elements = info.fmt->active_samples_per_line;

				active_video_line = 1;

				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1; info.ln <= 15; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
					for (info.ln = 16; info.ln <= 16; info.ln++) {
						info.xyz = &FIELD_1_ACTIVE;
						mkline(line_buffer, &info, VERT_BLANKING);
						p = pack(p, line_buffer, elements);
					}
				}

				info.xyz = &FIELD_1_ACTIVE;
				// 3 lines opt. video data
				for (info.ln = 17; info.ln <= 19; info.ln++) {
					mkline(line_buffer, &info, BLACK);
					p = pack(p, line_buffer, elements);
				}
				for (info.ln = 20; info.ln <= 259; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 260; info.ln <= 261; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
					info.xyz = &FIELD_2_VERT_BLANKING;
					// 7 lines vertical data
					for (info.ln = 262; info.ln <= 269; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
					// 9 lines opt. video data ?? // TODO have a look to SMPTE
					for (info.ln = 270; info.ln <= 278; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}

				}

				active_video_line = 0;
				// 3 lines opt. video data
				info.xyz = &FIELD_2_ACTIVE;
				for (info.ln = 279; info.ln <= 281; info.ln++) {
					mkline(line_buffer, &info, BLACK);
					p = pack(p, line_buffer, elements);
				}
				for (info.ln = 282; info.ln <= 521; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
					active_video_line += 2;
				}
				if (info.blanking) {
					info.xyz = &FIELD_2_VERT_BLANKING;
					for (info.ln = 522; info.ln <= 525; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
			}
		} else {

			/****************************************
			 * PROGRESSIVE
			 ****************************************/

			// start with first line numerber
			active_video_line = 0;

			if (info.fmt->lines_per_frame == 1125) {
				if (info.blanking) {
					elements = info.fmt->samples_per_line;
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1; info.ln <= 41; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				} else {
					elements = info.fmt->active_samples_per_line;
				}
				info.xyz = &FIELD_1_ACTIVE;
				for (info.ln = 42; info.ln <= 1121; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line++, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
				}
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1122; info.ln <= 1125; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
			} else {
				if (info.blanking) {
					elements = info.fmt->samples_per_line;
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 1; info.ln <= 25; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				} else {
					elements = info.fmt->active_samples_per_line;
				}
				info.xyz = &FIELD_1_ACTIVE;
				for (info.ln = 26; info.ln <= 745; info.ln++) {
					create_HD_SDI_Line(line_buffer, &info, active_video_line++, ACTIVE_VIDEO, vBuffer);
					p = pack(p, line_buffer, elements);
				}
				if (info.blanking) {
					info.xyz = &FIELD_1_VERT_BLANKING;
					for (info.ln = 746; info.ln <= 750; info.ln++) {
						create_HD_SDI_Line(line_buffer, &info, 0, VERT_BLANKING, vBuffer);
						p = pack(p, line_buffer, elements);
					}
				}
			}
		}
	}

	// sum of bytes that have already been written to file
	int bytes = 0;
	// store actual written bytes per 'write()
	int written_bytes = 0;

	/**
	 * WRITE BUFFER TO FILEHANDLE
	 **/
	// Write the complete frame to output
	// The "while" is necessary because the sdi device file does not take the complete frame at once
	written_bytes = 0;
	while (bytes < sdi_frame_size) {

		if ((written_bytes = write(fh_sdi_video, data + bytes, sdi_frame_size - bytes)) < 0) {
			fprintf(stderr, "\nunable to write SDI video.\n");
			return -1;
		}
		bytes += written_bytes;
	}

	// Check for events of the SDI board
	unsigned int val;
	if (ioctl(fh_sdi_video, SDI_IOC_TXGETEVENTS, &val) < 0) {
		// Maybe this is not an SDI device...
		//fprintf(stderr, "SDI VIDEO output:");
		//perror("unable to get the transmitter event flags");
	} else if (val) {
		if (val & SDI_EVENT_TX_BUFFER) {
			printf("SDI VIDEO driver transmit buffer queue underrun "
				"detected.\n");
			fflush(stdout);
		}
		if (val & SDI_EVENT_TX_FIFO) {
			printf("SDI VIDEO onboard transmit FIFO underrun detected.\n");
			fflush(stdout);
		}
		if (val & SDI_EVENT_TX_DATA) {
			printf("SDI VIDEO transmit data change detected.\n");
			fflush(stdout);
		}
	}

	// if available write audio data
	if (fh_sdi_audio) {

		// count writen bytes
		written_bytes = 0;

		// set number of samples and cut by 1600 if NTSC (handle problem of real time encoding of NTSC frequencies)
		size_t samples_total_per_track = audio_format->samples;
		uint16_t sample_number = 0;
		size_t channels_per_track_total = 2;
		uint8_t stream_number = 0;

		//printf("samples_total_per_track:%li\n", samples_total_per_track);

		// to write blockwise 2 samples of one track we must claculate the number of bytes we want to write per write-session
		// 2samples = 2x16Bit = 32Bit = 4Byte
		// 2samples = 2x32Bit = 64Bit = 8Byte
		// set total bytes per session
		size_t bytes_total = 0;
		bytes_total = audio_format->aformat == mlt_audio_s16 ? channels_per_track_total * sizeof(int16_t) : bytes_total;
		bytes_total = audio_format->aformat == mlt_audio_s32 ? channels_per_track_total * sizeof(int32_t) : bytes_total;

		// write all samples of all streams interleaved
		/**
		 * aBuffer[track0]+sample1
		 * aBuffer[track0]+sample2
		 * aBuffer[track1]+sample1
		 * aBuffer[track1]+sample2
		 * aBuffer[track.]+sample1
		 * aBuffer[track.]+sample2
		 *
		 * aBuffer[track0]+sample3
		 * aBuffer[track0]+sample4
		 * aBuffer[track1]+sample3
		 * aBuffer[track1]+sample4
		 * aBuffer[track.]+sample3
		 * aBuffer[track.]+sample4
		 *
		 * aBuffer[track0]+sample5
		 * aBuffer[track0]+sample6
		 * aBuffer[track1]+sample5
		 * aBuffer[track1]+sample6
		 * aBuffer[track.]+sample5
		 * aBuffer[track.]+sample6
		 **/
		int sum_written_bytes = 0;
		int sum_written_bytes_a = 0;
		int sum_written_bytes_b = 0;

		// write all samples per track
		while (sample_number < samples_total_per_track) {

			stream_number = 0;

			/**
			 * Because we have and write a fix number of audio streams to SDI board:
			 * we have a actual number of real audio tracks and a rest number of pseudo tracks
			 **/
			// write all streams
			while (stream_number < audio_streams) {

				// write for every stream n samples
				// n = number of channels per stream
				written_bytes = 0;
				while (written_bytes < bytes_total) {
					written_bytes += write(fh_sdi_audio, (uint8_t *) aBuffer[stream_number] + sample_number * bytes_total + written_bytes, bytes_total
							- written_bytes);
				}
				sum_written_bytes += written_bytes;
				sum_written_bytes_a += written_bytes;

				stream_number++;
			}

			// write pseudo tracks
			// now fill rest of audio tracks(AES frames) with NULL or copy of first track
			while (stream_number < audio_format->channels/2) {

				// write for every stream n samples
				// n = number of channels per stream
				written_bytes = 0;
				while (written_bytes < bytes_total) {
					written_bytes += write(fh_sdi_audio, (uint8_t *) aBuffer[0] + sample_number * bytes_total + written_bytes, bytes_total - written_bytes);
				}
				sum_written_bytes += written_bytes;
				sum_written_bytes_b += written_bytes;

				stream_number++;
			}

			sample_number++;

			// Check for events of the SDI audio device
			unsigned int val;
			if (ioctl(fh_sdi_audio, SDIAUDIO_IOC_TXGETEVENTS, &val) < 0) {
				//Maybe this is not an SDI device...
				//				fprintf(stderr, "SDI AUDIO output:");
				//				perror("unable to get the transmitter event flags");
			} else if (val) {
				if (val & SDIAUDIO_EVENT_TX_BUFFER) {
					printf("SDI AUDIO driver transmit buffer queue underrun "
						"detected.\n");
				}
				if (val & SDIAUDIO_EVENT_TX_FIFO) {
					printf("SDI AUDIO onboard transmit FIFO underrun detected.\n");
				}
				if (val & SDIAUDIO_EVENT_TX_DATA) {
					printf("SDI AUDIO transmit data change detected.\n");
				}
			}
		}
	}

	return getDBN(my_DBN);
} // end sdimaster_playout()


//****************************************************************************************
//***************************  Create Line  **********************************************
//****************************************************************************************

/** generate one SDI line
 * @param *buf: buffer to hold the line
 * @param field: size of the video Buffer
 * @param active: v-blank or active-video
 * @param *video_buffer: video buffer
 * @param *audio_buffer2: 1.audio buffer ch1-ch2
 * @param *audio_buffer1: 2.audio buffer ch2-ch3
 * @param line: linenumber
 * @param AudioGroupCounter: count written AudioGroup
 * @param AudioGroups2Write: number of samples to write
 * @param audio_streams: number of audio streams to integrate
 */
static inline int create_SD_SDI_Line(uint16_t *buf, const struct line_info *info, int field, int active, uint8_t *video_buffer,
		int16_t audio_buffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES], int linenumber_sdiframe, int active_video_line, int my_DBN, int16_t AudioGroupCounter,
		int16_t AudioGroups2Write, int audio_streams) {

	// write line with TRS(EAV) ANC(audio) TRS(SAV) activeVideo(CbY1CrY2)
	//					*************************************************************************
	// 625 lines:		| EAV |     ANC      | SAV |		[CbY1CrY2]							|
	//					*************************************************************************
	// 1728 SDI-words:	| 4   | 	280		 | 4   |		720+360+360=1440					|
	//					*************************************************************************

	// points to current position in line
	uint16_t *p = buf;

	//#########################################################################################
	/* TRS Timing Reference Signal for EAV
	 *		[3ff]
	 *		[000]
	 *		[000]
	 *		[XYZ-Wort]
	 * */

	*p++ = 0x3ff;
	*p++ = 0x000;
	*p++ = 0x000;
	*p++ = info->xyz->eav;
	//#########################################################################################

	/* ANC Ancillary Data with AES
	 *
	 *	[ADF][ADF][ADF][DID][DBN][DC][UDW]...[UDW][CS]
	 *
	 * */
	// write ANC Data and get number of samples are written
	// step with `p` += to the number of written samples

	//printf("audio_streams:%i\n",audio_streams);

	// 1 stream, Audio Group 1 with AES Frame 1 - 2
	if (audio_streams == 1) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[0], AudioGroupCounter, AudioGroups2Write);
	}
	// 2 streams, Audio Group 1 with AES Frame 1 - 2
	if (audio_streams == 2) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
	}
	// 3 streams, Audio Group 2 with AES Frame 1 - 4
	if (audio_streams == 3) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[2], AudioGroupCounter, AudioGroups2Write);
	}
	// 4 streams, Audio Group 2 with AES Frame 1 - 4
	if (audio_streams == 4) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write);
	}
	// 5 streams, Audio Group 3 with AES Frame 1 - 6
	if (audio_streams == 5) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FB, my_DBN, audio_buffer[4], audio_buffer[4], AudioGroupCounter, AudioGroups2Write);
	}
	// 6 streams, Audio Group 3 with AES Frame 1 - 6
	if (audio_streams == 6) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FB, my_DBN, audio_buffer[4], audio_buffer[5], AudioGroupCounter, AudioGroups2Write);
	}
	// 7 streams, Audio Group 4 with AES Frame 1 - 7
	if (audio_streams == 7) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FB, my_DBN, audio_buffer[4], audio_buffer[5], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x2F9, my_DBN, audio_buffer[6], audio_buffer[6], AudioGroupCounter, AudioGroups2Write);
	}
	// 8 streams, Audio Group 4 with AES Frame 1 - 7
	if (audio_streams == 8) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x1FB, my_DBN, audio_buffer[4], audio_buffer[5], AudioGroupCounter, AudioGroups2Write);
		p += writeANC(p, linenumber_sdiframe, 0x2F9, my_DBN, audio_buffer[6], audio_buffer[7], AudioGroupCounter, AudioGroups2Write);
	}

	// Fill ANC data in until the end (position(p) to `ANCILLARY_DATA_SAMPLES`)
	while (p < (buf + ANCILLARY_DATA_SAMPLES + 4)) {
		// video color: black
		*p++ = 0x200;
		*p++ = 0x040;
	}
	//#########################################################################################
	// TRS Timing Reference Signal for SAV
	*p++ = 0x3ff;
	*p++ = 0x000;
	*p++ = 0x000;
	*p++ = info->xyz->sav;
	//#########################################################################################


	// Because we skip the first line of video, it can happen that we read too far in the buffer
	if (active_video_line >= info->fmt->active_lines_per_frame) {
		active_video_line = info->fmt->active_lines_per_frame - 1; // in SD PAL was set 575
	}
	//Index of the start of the current line in the video_buffer
	int start_of_current_line = active_video_line * info->fmt->active_samples_per_line;

	// If VBlank then fill the line with 0x200 and 0x040 (total black)
	switch (active) {
	default:
	case VERT_BLANKING:
		while (p < (buf + info->fmt->samples_per_line)) {
			*p++ = 0x200;
			*p++ = 0x040;
		}
		break;
	case ACTIVE_VIDEO:

		// Insert the video into the line
		while (p < (buf + info->fmt->samples_per_line)) { // fill the rest of the line with active video

			// shift "<< 2" because 8 bit data in 10 bit word

			*p = video_buffer[start_of_current_line + ((p - 288) - buf) + 1] << 2; // Cb
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040; // check values
			if (*(p - 1) > 0x3c0)
				*(p - 1) = 0x3c0;
			*p = video_buffer[start_of_current_line + ((p - 288) - buf) - 1] << 2; // Y1
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040;
			if (*(p - 1) > 0x3ac)
				*(p - 1) = 0x3ac;
			*p = video_buffer[start_of_current_line + ((p - 288) - buf) + 1] << 2; // Cr
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040;
			if (*(p - 1) > 0x3c0)
				*(p - 1) = 0x3c0;
			*p = video_buffer[start_of_current_line + ((p - 288) - buf) - 1] << 2; // Y2
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040;
			if (*(p - 1) > 0x3ac)
				*(p - 1) = 0x3ac;

		}
		break;
	}
	return 0;
}

/**
 * create_HD_SDI_Line - generate one line
 * @buf: pointer to a buffer
 * @info: pointer to a line information structure
 * @active_video_line
 * @active:
 * @video_buffer: pattern
 *
 * Returns a negative error code on failure and zero on success.
 **/
static inline int create_HD_SDI_Line(uint16_t *buf, const struct line_info *info, uint16_t active_video_line, unsigned int active, uint8_t *video_buffer) {
	uint16_t *p = buf, *endp, ln;
	uint16_t samples = info->blanking ? info->fmt->samples_per_line : info->fmt->active_samples_per_line;

	if (active_video_line >= info->fmt->active_lines_per_frame) {
		active_video_line = info->fmt->active_lines_per_frame - 1;
	}

	int start_of_current_line = active_video_line * info->fmt->active_samples_per_line;

	if (info->blanking) {

		// write line with TRS(EAV) ANC(audio) TRS(SAV) activeVideo(CbY1CrY2)
		// Example SD PAL:
		//                  *************************************************************************
		// 625 lines:       | EAV |     ANC      | SAV |        [CbY1CrY2]                          |
		//                  *************************************************************************
		// 1728 SDI-words:  | 4   |     280	     | 4   |        720+360+360=1440                    |
		//					*************************************************************************

		// write line with TRS(EAV) ANC(audio) TRS(SAV) activeVideo(CbY1CrY2)
		// Example HD 1080i:
		//                  *************************************************************************
		// 1125 lines:      | EAV | LN  | CRC |	ANC | SAV | [CbY1CrY2]                              |
		//                  *************************************************************************
		// 5280 SDI-words:  | 6   | 4   | 4   | 280	| 6   |	1920+720+720=3840                       |
		//                  *************************************************************************

		if (info->fmt == &FMT_576i50) {
			/* EAV */
			*p++ = 0x3ff;
			*p++ = 0x000;
			*p++ = 0x000;
			*p++ = info->xyz->eav;
		} else {
			/* EAV */
			*p++ = 1023;
			*p++ = 1023;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = info->xyz->eav;
			*p++ = info->xyz->eav;
			/* LN */
			ln = ((info->ln & 0x07f) << 2) | (~info->ln & 0x040) << 3;
			*p++ = ln;
			*p++ = ln;
			ln = ((info->ln & 0x780) >> 5) | 0x200;
			*p++ = ln;
			*p++ = ln;
			/* CRC, added by serializer */
			*p++ = 512;
			*p++ = 64;
			*p++ = 512;
			*p++ = 64;

		}

		/* Horizontal blanking */
		while (p < (buf + info->fmt->samples_per_line - info->fmt->active_samples_per_line - 4)) {
			*p++ = 512;
			*p++ = 64;
			*p++ = 512;
			*p++ = 64;
		}

		if (info->fmt == &FMT_576i50) {
			/* SAV */
			*p++ = 0x3ff;
			*p++ = 0x000;
			*p++ = 0x000;
			*p++ = info->xyz->sav;
		} else {
			/* SAV */
			*p++ = 1023;
			*p++ = 1023;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = 0;
			*p++ = info->xyz->sav;
			*p++ = info->xyz->sav;
		}
	}
	/* Active region */
	endp = p;

	switch (active) {
	default:
	case VERT_BLANKING:
		while (p < (buf + samples)) {
			*p++ = 512;
			*p++ = 64;
			*p++ = 512;
			*p++ = 64;
		}
		break;
	case ACTIVE_VIDEO:

	{

		while (p < (buf + samples)) {

			*p = video_buffer[start_of_current_line + (p - buf) + 1] << 2; // Cb
			p++;
			//check values, this needs a lot of resources
			//			if (*(p - 1) < 0x040)
			//				*(p - 1) = 0x040;
			//			if (*(p - 1) > 0x3c0)
			//				*(p - 1) = 0x3c0;
			//
			*p = video_buffer[start_of_current_line + (p - buf) - 1] << 2; // Y1
			p++;
			//			if (*(p - 1) < 0x040)
			//				*(p - 1) = 0x040;
			//			if (*(p - 1) > 0x3ac)
			//				*(p - 1) = 0x3ac;
			//
			*p = video_buffer[start_of_current_line + (p - buf) + 1] << 2; // Cr
			p++;
			//			if (*(p - 1) < 0x040)
			//				*(p - 1) = 0x040;
			//			if (*(p - 1) > 0x3c0)
			//				*(p - 1) = 0x3c0;
			//
			*p = video_buffer[start_of_current_line + (p - buf) - 1] << 2; // Y2
			p++;
			//			if (*(p - 1) < 0x040)
			//				*(p - 1) = 0x040;
			//			if (*(p - 1) > 0x3ac)
			//				*(p - 1) = 0x3ac;
		}
	}
		break;
	}
	return 0;
}

static int writeANC(uint16_t *p, int videoline_sdiframe, uint16_t DID, int my_DBN, int16_t *audio_buffer_A, int16_t *audio_buffer_B, int16_t AudioGroupCounter,
		int16_t AudioGroups2Write) {

	/**
	 * ANC Ancillary Data (vgl. SMPTE 291-M page 6 )
	 * [ADF][ADF][ADF][DID][DBN][DC][UDW]...[UDW][CS]
	 *
	 **/

	// save only current position for return value
	uint16_t *pp = p;
	// 16bit buffer to write temporarily 10bit word
	uint16_t buffer = 0; // set all explicit to zero, special the bit9 for parity
	// parity_counter
	int8_t parity_counter = 0;

	if (AudioGroups2Write > 0) {

		// 3 ADF	(Ancillary Data Flag)
		*p++ = 0x000;
		*p++ = 0x3FF;
		*p++ = 0x3FF;

		// 1 DID	(Data Identification)
		// save DID for checker()
		uint16_t *DID_pointer = p;
		*p++ = DID;// (AES Audio Data, Group
		//		*p++ = 0x2FF; // (AES Audio Data, Group1=0x2FF)
		//		*p++ = 0x1FD;	// (AES Audio Data, Group2=0x1FD)
		//		*p++ = 0x1FB;	// (AES Audio Data, Group3=0x1FB)
		//		*p++ = 0x2F9;	// (AES Audio Data, Group4=0x2F9)

		// 1 DBN	(Data Block Number) inactiv: 1000000000 b9,b8,b7-b0	; SMPTE 272-M chapter15.1
		//		*p++ = 0x200;

		// 1 DBN (dynamic version0.1-beta ), should start with previus DBN of SDI-Frame
		//	-need "previus DBN" or "current framenumber"
		//		SDI-LINE:	DBN:
		//		[1]			[1]		<< start sdi frame
		//		[2]			[2]
		//		[.]			[.]
		//		[255]		[255]
		//		[256]		[1]
		//		[257]		[2]
		//		[.]			[.]
		//		[510]		[255]
		//		[511]		[1]
		//		[512]		[2]
		//		[.]			[.]
		//		[625]		[115]	<< end sdi frame
		//		[1]			[116]	<< start sdi frame
		// Accuracy of videoline_sdiframe(1 up to 625) to 8bit (1-255)
		//buffer = ((videoline_sdiframe-1) % 255)+1;
		buffer = my_DBN;
		parity_counter = 0;
		// count binary ones for parity
		int i = 0;
		for (i = 0; i < 8; i++) {
			if (buffer & (1 << i))
				parity_counter++;
		}
		if ((parity_counter % 2) == 0) { //else leave the 0
			buffer += 512; // 10 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		} else {
			buffer += 256; // 01 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		}
		*p++ = buffer;

		// 1 DC 	(Data Counter)
		// number of UDW = AudioGroups2Write x 2AESFrames x 2channesl x 3words(X,X+1,X+2)
		buffer = AudioGroups2Write * 2* 2* 3 ; parity_counter= 0;
		// count binary ones for parity
		for (i=0; i<8; i++) {
			if (buffer & (1 << i))
			parity_counter++;
		}
		if ((parity_counter%2)==0) { //else leave the 0
			buffer+= 512; // 10 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		} else {
			buffer+= 256; // 01 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		}
		*p++ = buffer;

		int16_t sample_number=0;
		int16_t counter = 0;
		// write subframes:
		// = n x 1 AudioGroup
		// = n x 2 x 1AESFrame
		// = n x 2 x 2samples
		// = 4 samples
		// = 4 x 3words
		while (counter < AudioGroups2Write*2) { /* 4:3 */

			// write one Audio Group with 4 x AES subframes
			// ( samples for ch01,ch02,ch03,ch04 or ch05,ch06,ch07,ch08 or ch09,ch10,ch11,ch12 or ch13,ch14,ch15,ch16)
			// and use audio_buffer_A(stereo) and audio_buffer_B(stereo)
			// `pack_AES_subframe()` write 3 ANC words (3*10bit), also 1 sample

			sample_number=(AudioGroupCounter*2)+ counter;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 1),
					getZBit(sample_number/2), 0, &audio_buffer_A[sample_number]); // left
			p+=3; // step 3 words

			sample_number=(AudioGroupCounter*2)+ counter+1;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 2),
					getZBit(sample_number/2), 1, &audio_buffer_A[sample_number]); // right
			p+=3;

			sample_number=(AudioGroupCounter*2)+ counter;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 3),
					getZBit(sample_number/2), 2, &audio_buffer_B[sample_number]); // left
			p+=3;

			sample_number=(AudioGroupCounter*2)+ counter+1;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 4),
					getZBit(sample_number/2), 3, &audio_buffer_B[sample_number]); // right
			p+=3;
			counter+=2;
		}

		// 1 CS		(Checksum from DID - UDW)
		*p++ = checker(DID_pointer);

		// fill ANC with one dummy for videocolor black
		// rest until end of `ANCILLARY_DATA_SAMPLES` will be fill in a loop after call this function
		*p++ = 0x040;
	}
	return p-pp;
}

// calculate checksumm of ANC (SMPTE 272-M 15.3 Checksum (CS))
static uint16_t checker(uint16_t *DID_pointer) {

	// Checksumm
	uint16_t cs = 0x00;

	// DID - Datablock Identification
	cs += (*DID_pointer++) & 0x1FF; // 9 x LSB

	// DBN - Datablock Number
	cs += (*DID_pointer++) & 0x1FF; // 9 x LSB

	// DC - DataCounter
	cs += (*DID_pointer) & 0x1FF; // 9 x LSB

	// store address of DC an ad to the real value of DC
	// DataCounter store
	// ´ende´ point to DataCounter
	uint16_t *ende = DID_pointer;
	// ´ende´ point to last field
	ende += (*DID_pointer) & 0xFF; // without parity-Bit and ¬9-Bit

	DID_pointer++;

	// while DID_pointer point to smaller addres like 'ende'
	while (DID_pointer <= ende) {
		cs += (*DID_pointer++) & 0x1FF; // 9 x LSB
	}

	// limit to 9Bit, because of overflow of sum
	cs = cs & 0x1FF;

	// set bit10 NOT bit9:
	// - cs invert
	// - & with bitmask '01 0000 0000'
	// - shift rest (1xbit)to left
	// - add to cs
	cs += ((~cs) & 0x100) << 1;

	return cs;
} // end checker


/**
 * pack 16bit in AES subframe with 3 words (30 bit) and write in ´*p´
 * 10bit-words --> [X],[X+1], [X+2] implements 20bit for audio
 *
 *			BIT	  9,     8,   7,   6,   5,   4,   3,   2,   1,   0
 *				  #####  ###  ###  ###  ###  ###  ###  ###  ###  ###
 *	[X]		:	[ !bit8, a5,  a4,  a3,  a2,  a1,  a0,  ch0, ch1, z   ],
 *	[X+1]	:	[ !bit8, a14, a13, a12, a11, a10, a9,  a8,  a7 , a6  ],
 *	[X+2]	:	[ !bit8, P,   C,   U,   V,   a19, a18, a17, a16, a15 ]
 *
 * @param *p: Pointer to SDI frame buffer
 * @param c: value of AES subframe Channel Status Bit
 * @param z: value of AES subframe
 * @param ch: channel od AES subframe (value:0,1,2,3)
 * @param *audio_samplex: pointer to the audio buffer
 **/
static int pack_AES_subframe(uint16_t *p, int8_t c, int8_t z, int8_t ch, int16_t *audio_samplex) {

	/**
	 *  NOTE: WE JUST SUPPORT ONLY 16BIT SAMPLE SIZE
	 **/

	// push 16bit up to 20bit(32bit)
	int32_t audio_sample = *audio_samplex;
	audio_sample = audio_sample << 4; // Shift by 4 (louder)

	// parity_counter
	int8_t parity_counter = 0;

	// 16bit buffer to write 10bit of [X]word,[X+1]word,[X+2]word,
	uint16_t buffer = 0;

	//#########################################################
	//### WORD X  ############################################
	//#########################################################
	// word X: !bit8, a5, a4, a3, a2, a1, a0, ch1, ch0, z
	// SMPTE 272M s.7
	buffer = z; // z bit every 192bit = 1
	buffer += ch << 1; // ch1 - ch0
	buffer += (audio_sample & 0x3f) << 3; // a5 - a0
	buffer += ((~buffer) & 0x100) << 1; // !bit8

	// write word ´X´
	*p++ = buffer;

	// count ones
	int i = 0;
	for (i = 0; i < 9; i++) {
		if (buffer & 1 << i)
			parity_counter++;
	}

	//#########################################################
	//### WORD X+1 ############################################
	//#########################################################
	// word X+1: !bit8, a14, a13, a12, a11, a10, a9, a8, a7, a6
	// SMPTE 272M s.7
	buffer = 0;
	buffer += (audio_sample >> 6) & 0x1ff; // a14 - a6
	buffer += ((~buffer) & 0x100) << 1; // !bit8

	// write word ´X+1´
	*p++ = buffer;

	// count ones (zähle Einsen)
	i = 0;
	for (i = 0; i < 9; i++) {
		if (buffer & 1 << i)
			parity_counter++;
	}

	//#########################################################
	//### WORD X+2 ############################################
	//#########################################################
	// word X+2: !bit8, P, C, U, V, a19, a18, a17, a16, a15
	// SMPTE 272M s.7
	buffer = 0;
	buffer += (audio_sample >> 15) & 0x01F; // a15 - a19
	// default of [V][U][C] bits = `0`
	//buffer += 1<<5;	// V (AES sample validity bit)
	//buffer += 1<<6;	// U (AES user bit)
	//buffer += 1<<7;	// C (AES audio channel status bit)
	buffer += c << 7; // C (AES audio channel status bit)

	// count ones (zähle Einsen)
	for (i = 0; i < 8; i++) {
		if (buffer & 1 << i)
			parity_counter++;
	}

	//	if (!parity_counter%2) //else leave the 0
	//		buffer+= 1 << 8; // P (AES even parity bit)
	//
	//	buffer += ((~buffer) & 0x100 )<<1; // !bit8
	if ((parity_counter % 2) == 0) { //else leave the 0
		buffer += 512; // 10 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
	} else {
		buffer += 256; // 01 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
	}
	*p++ = buffer;

	// write word ´X+2´
	*p++ = buffer;

	return 1;
}

static uint8_t getZBit(int sample_number) {

	// start in SDI line 6 also 18samples later
	//sample_number+=192-18;

	if (sample_number % 192 == 0) {
		//printf("1 %i\n", sample_number);
		return 1;
	} else {
		//printf("0");
		return 0;
	}
}

static uint8_t getChannelStatusBit(uint16_t sample_number, uint8_t ch) {

	// return value
	uint8_t AESChannelStatusBit = 0;

	// start in SDI line 6 also 18samples later
	//AESChannelStatusBit=((sample_number+192-18)%192);
	// interval in 192bit
	AESChannelStatusBit = sample_number % 192;

	// when mulichannelmode is true
	if (AESChannelStatusBitArray[31] == 1) {
		// set bits for channel
		if (AESChannelStatusBit == 30 && ch == 2)
			return 1;
		if (AESChannelStatusBit == 30 && ch == 4)
			return 1;
		if (AESChannelStatusBit == 29 && (ch == 4))
			return 1;
		if (AESChannelStatusBit == 29 && (ch == 3))
			return 1;
	}
	return AESChannelStatusBitArray[AESChannelStatusBit];
}

static int16_t getNumberOfAudioGroups2Write(int linenumber) {

	// `4:3_VTR`-distribution
	if (linenumber >= 11 && linenumber <= 95) {
		if ((linenumber - 11) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else if (linenumber >= 108 && linenumber <= 220) {
		if ((linenumber - 10) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else if (linenumber >= 233 && linenumber <= 345) {
		if ((linenumber - 9) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else if (linenumber >= 358 && linenumber <= 470) {
		if ((linenumber - 8) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else if (linenumber >= 483 && linenumber <= 595) {
		if ((linenumber - 7) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else if (linenumber >= 608 && linenumber <= 622) {
		if ((linenumber - 6) % 14 == 0) {
			return 4;
		} else {
			return 3;
		}
	} else {
		return 3;
	}

	//	// `4:3`-distribution
	//	if(linenumber<=315){
	//		if(linenumber>=6 && linenumber<=8){
	//			return 0;
	//		}
	//		if((linenumber+5)%10==0){
	//			return 4;
	//		}else{
	//			return 3;
	//		}
	//	}else{
	//		if(linenumber>=319 && linenumber<=321){
	//			return 0;
	//		}
	//		if((linenumber-8)%10==0){
	//			return 4;
	//		}else{
	//			return 3;
	//		}
	//	}

	//	// full-distribution
	//	if(linenumber<=45){
	//		return 4;
	//	}else{
	//		return 3;
	//	}

	//	// fullhalf-distribution
	//	if (linenumber==625)
	//		return 4;
	//
	//	if (linenumber%14==0) {
	//		return 4;
	//	} else {
	//		return 3;
	//	}

}
static uint8_t getDBN(int my_DBN) {

	return ((my_DBN - 1) % 255) + 1;
}

/**
 * pack8 - pack a line of 8-bit data
 * @outbuf: pointer to the output buffer
 * @inbuf: pointer to the input buffer
 * @count: number of elements in the buffer
 *
 * Returns a pointer to the next output location.
 **/
static inline uint8_t *
pack8(uint8_t *outbuf, uint16_t *inbuf, size_t count) {
	uint16_t *inp = inbuf;
	uint8_t *outp = outbuf;

	while (inp < (inbuf + count)) {
		*outp++ = *inp++ >> 2;
	}
	return outp;
}

/**
 * pack10 - pack a line of 10-bit data
 * @outbuf: pointer to the output buffer
 * @inbuf: pointer to the input buffer
 * @count: number of elements in the buffer
 *
 * Returns a pointer to the next output location.
 **/
static inline uint8_t * pack10(uint8_t *outbuf, uint16_t *inbuf, size_t count) {

	uint16_t *inp = inbuf;
	uint8_t *outp = outbuf;

	while (inp < (inbuf + count)) {
		*outp++ = *inp & 0xff;
		*outp = *inp++ >> 8;
		*outp++ += (*inp << 2) & 0xfc;
		*outp = *inp++ >> 6;
		*outp++ += (*inp << 4) & 0xf0;
		*outp = *inp++ >> 4;
		*outp++ += (*inp << 6) & 0xc0;
		*outp++ = *inp++ >> 2;
	}

	return outp;
}
/**
 * pack_v210 - pack a line of v210 data
 * @outbuf: pointer to the output buffer
 * @inbuf: pointer to the input buffer
 * @count: number of elements in the buffer
 *
 * Returns a pointer to the next output location.
 **/
static inline uint8_t * pack_v210(uint8_t *outbuf, uint16_t *inbuf, size_t count) {

	uint16_t *inp = inbuf;
	uint8_t *outp = outbuf;

	count = (count / 96) * 96 + ((count % 96) ? 96 : 0);
	while (inp < (inbuf + count)) {
		*outp++ = *inp & 0xff;
		*outp = *inp++ >> 8;
		*outp++ += (*inp << 2) & 0xfc;
		*outp = *inp++ >> 6;
		*outp++ += (*inp << 4) & 0xf0;
		*outp++ = *inp++ >> 4;
	}
	return outp;
}

// Clean up
static int sdimaster_close() {

	free(line_buffer);
	free(data);

	if (fh_sdi_audio)
		close(fh_sdi_audio);
	if (fh_sdi_video)
		close(fh_sdi_video);

	return 1;
}

/**
 * mkline - generate one line
 * @buf: pointer to a buffer
 * @info: pointer to a line information structure
 * @pattern: pattern
 *
 * Returns a negative error code on failure and zero on success.
 **/
static int mkline(unsigned short int *buf, const struct line_info *info, unsigned int pattern) {
	const unsigned int b = 205;
	unsigned short int *p = buf, *endp;
	unsigned int samples = info->blanking ? info->fmt->samples_per_line : info->fmt->active_samples_per_line;

	if (info->blanking) {
		/* EAV */
		*p++ = 0x3ff;
		*p++ = 0x000;
		*p++ = 0x000;
		*p++ = info->xyz->eav;
		/* Horizontal blanking */
		while (p < (buf + 272)) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* SAV */
		*p++ = 0x3ff;
		*p++ = 0x000;
		*p++ = 0x000;
		*p++ = info->xyz->sav;
	}
	/* Active region */
	endp = p;
	switch (pattern) {
	default:
	case VERT_BLANKING:
		while (p < (buf + samples)) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		break;
	case BLACK:
		while (p < (buf + samples)) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		break;
	case MAIN_SET:
		/* 75% gray */
		endp += b + 1;
		while (p < endp) {
			*p++ = 512;
			*p++ = 721;
			*p++ = 512;
			*p++ = 721;
		}
		/* 75% yellow */
		endp += b + 1;
		while (p < endp) {
			*p++ = 176;
			*p++ = 646;
			*p++ = 567;
			*p++ = 646;
		}
		/* 75% cyan */
		endp += b + 1;
		while (p < endp) {
			*p++ = 625;
			*p++ = 525;
			*p++ = 176;
			*p++ = 525;
		}
		/* 75% green */
		endp += b - 1;
		while (p < endp) {
			*p++ = 289;
			*p++ = 450;
			*p++ = 231;
			*p++ = 450;
		}
		/* 75% magenta */
		endp += b + 1;
		while (p < endp) {
			*p++ = 735;
			*p++ = 335;
			*p++ = 793;
			*p++ = 335;
		}
		/* 75% red */
		endp += b + 1;
		while (p < endp) {
			*p++ = 399;
			*p++ = 260;
			*p++ = 848;
			*p++ = 260;
		}
		/* 75% blue */
		while (p < (buf + samples)) {
			*p++ = 848;
			*p++ = 139;
			*p++ = 457;
			*p++ = 139;
		}
		break;
	case CHROMA_SET:
		/* 75% blue */
		endp += b + 1;
		while (p < endp) {
			*p++ = 848;
			*p++ = 139;
			*p++ = 457;
			*p++ = 139;
		}
		/* black */
		endp += b + 1;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* 75% magenta */
		endp += b + 1;
		while (p < endp) {
			*p++ = 735;
			*p++ = 335;
			*p++ = 793;
			*p++ = 335;
		}
		/* black */
		endp += b - 1;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* 75% cyan */
		endp += b + 1;
		while (p < endp) {
			*p++ = 625;
			*p++ = 525;
			*p++ = 176;
			*p++ = 525;
		}
		/* black */
		endp += b + 1;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* 75% gray */
		while (p < (buf + samples)) {
			*p++ = 512;
			*p++ = 721;
			*p++ = 512;
			*p++ = 721;
		}
		break;
	case BLACK_SET:
		/* -I */
		endp += 257;
		while (p < endp) {
			*p++ = 624;
			*p++ = 231;
			*p++ = 390;
			*p++ = 231;
		}
		/* white */
		endp += 257;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 940;
			*p++ = 0x200;
			*p++ = 940;
		}
		/* +Q */
		endp += 257;
		while (p < endp) {
			*p++ = 684;
			*p++ = 177;
			*p++ = 591;
			*p++ = 177;
		}
		/* black */
		endp += 257;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* blacker than black */
		endp += 68;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 29;
			*p++ = 0x200;
			*p++ = 29;
		}
		/* black */
		endp += 68 + 2;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		/* whiter than black */
		endp += 68;
		while (p < endp) {
			*p++ = 0x200;
			*p++ = 99;
			*p++ = 0x200;
			*p++ = 99;
		}
		/* black */
		while (p < (buf + samples)) {
			*p++ = 0x200;
			*p++ = 0x040;
			*p++ = 0x200;
			*p++ = 0x040;
		}
		break;
	}
	return 0;
}

static int setSDIVideoProperties(enum sdi_setting_video_e setting, char * value, char * device) {

	const char fmt[] = "/sys/class/sdivideo/sdivideo%cx%i/%s";
	struct stat buf;
	int num;
	char type, name[256], data[256];
	char *endptr;

	/* Get the sysfs info */
	memset(&buf, 0, sizeof(buf));

	/**
	 * Stat the file, fills the structure with info about the file
	 * Get the major number from device node
	 **/
	if (stat(device, &buf) < 0) {
		fprintf(stderr, "%s: ", device);
		perror("unable to get the file status");
		return -1;
	}

	/* Check if it is a character device or not */
	if (!S_ISCHR (buf.st_mode)) {
		fprintf(stderr, "%s: not a character device\n", device);
		return -1;
	}

	/* Check the minor number to determine if it is a receive or transmit device */
	type = (buf.st_rdev & 0x0080) ? 'r' : 't';

	/* Get the receiver or transmitter number */
	num = buf.st_rdev & 0x007f;

	/* Build the path to sysfs file */
	snprintf(name, sizeof(name), fmt, type, num, "dev");
	memset(data, 0, sizeof(data));

	/* Read sysfs file (dev) */
	if (util_read(name, data, sizeof(data)) < 0) {
		fprintf(stderr, "%s: ", device);
		perror("unable to get the device number");
		return -1;
	}
	/* Compare the major number taken from sysfs file to the one taken from device node */
	if (strtoul(data, &endptr, 0) != (buf.st_rdev >> 8)) {
		fprintf(stderr, "%s: not a SMPTE 292M/SMPTE 259M-C device\n", device);
		return -1;
	}
	if (*endptr != ':') {
		fprintf(stderr, "%s: error reading %s\n", device, name);
		return -1;
	}

	// Which setting do we write
	if (setting == SETTING_BUFFER_NUMBER_VIDEO) {
		snprintf(name, sizeof(name), fmt, type, num, "buffers");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the number of buffers");
			return -1;
		}
		printf("\tSet number of buffers = %s\n", value);
	} else if (setting == SETTING_BUFFER_SIZE_VIDEO) {
		snprintf(name, sizeof(name), fmt, type, num, "bufsize");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the buffer size");
			return -1;
		}
		printf("\tSet buffer size = %s Bytes\n", value);
	} else if (setting == SETTING_CLOCK_SOURCE) {
		snprintf(name, sizeof(name), fmt, type, num, "clock_source");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the clock source");
			return -1;
		}
		printf("\tSet clock source = %s\n", value);
	} else if (setting == SETTING_DATA_MODE) {
		snprintf(name, sizeof(name), fmt, type, num, "mode");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the interface operating mode");
			return -1;
		}
		printf("\tSet data mode = %s\n", value);
	} else if (setting == SETTING_FRAME_MODE) {
		snprintf(name, sizeof(name), fmt, type, num, "frame_mode");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the interface frame mode");
			return -1;
		}
		printf("\tSet frame mode = %s\n", value);
	}

	return 0;

}

static int setSDIAudioProperties(enum sdi_setting_audio_e setting, char * value, char * device) {
	const char fmt[] = "/sys/class/sdiaudio/sdiaudio%cx%i/%s";
	struct stat buf;
	int num;
	char type, name[256], data[256];
	char *endptr;

	/* Get the sysfs info */
	memset(&buf, 0, sizeof(buf));
	if (stat(device, &buf) < 0) {
		fprintf(stderr, "%s: ", device);
		perror("unable to get the file status");
		return -1;
	}
	if (!S_ISCHR (buf.st_mode)) {
		fprintf(stderr, "%s: not a character device\n", device);
		return -1;
	}
	type = (buf.st_rdev & 0x0080) ? 'r' : 't';
	num = buf.st_rdev & 0x007f;
	snprintf(name, sizeof(name), fmt, type, num, "dev");
	memset(data, 0, sizeof(data));
	if (util_read(name, data, sizeof(data)) < 0) {
		fprintf(stderr, "%s: ", device);
		perror("unable to get the device number");
		return -1;
	}

	if (strtoul(data, &endptr, 0) != (buf.st_rdev >> 8)) {
		fprintf(stderr, "%s: not an audio device\n", device);
		return -1;
	}
	if (*endptr != ':') {
		fprintf(stderr, "%s: error reading %s\n", device, name);
		return -1;
	}

	if (setting == SETTING_BUFFER_NUMBER_AUDIO) {
		snprintf(name, sizeof(name), fmt, type, num, "buffers");
		snprintf(data, sizeof(data), "%s\n", value);

		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the number of buffers");
			return -1;
		}
		printf("\tSet number of buffers = %s\n", value);
	} else if (setting == SETTING_BUFFER_SIZE_AUDIO) {
		snprintf(name, sizeof(name), fmt, type, num, "bufsize");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the buffer size");
			return -1;
		}
		printf("\tSet buffer size = %s Bytes\n", value);
	} else if (setting == SETTING_SAMPLE_SIZE) {
		snprintf(name, sizeof(name), fmt, type, num, "sample_size");
		snprintf(data, sizeof(data), "%s\n", value);
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the interface audio sample size");
			return -1;
		}
		switch (atol(value)) {
		case SDIAUDIO_CTL_AUDSAMP_SZ_16:
			printf("\tAssuming 16-bit audio.\n");
			break;
		case SDIAUDIO_CTL_AUDSAMP_SZ_24:
			printf("\tAssuming 24-bit audio.\n");
			break;
		case SDIAUDIO_CTL_AUDSAMP_SZ_32:
			printf("\tAssuming 32-bit audio.\n");
			break;
		default:
			printf("\tSet audio sample size = %lu.\n", atol(value));
			break;
		}
	} else if (setting == SETTING_SAMPEL_RATE) {
		snprintf(name, sizeof(name), fmt, type, num, "sample_rate");
		snprintf(data, sizeof(data), "%lu\n", atol(value));
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set the interface audio sample rate");
			return -1;
		}
		switch (atoi(value)) {
		case 32000:
			printf("\tAssuming 32 kHz audio.\n");
			break;
		case 44100:
			printf("\tAssuming 44.1 kHz audio.\n");
			break;
		case 48000:
			printf("\tAssuming 48 kHz audio.\n");
			break;
		default:
			printf("\tSet audio sample rate = %lu.\n", atol(value));
			break;
		}
	} else if (setting == SETTING_CHANNELS) {
		snprintf(name, sizeof(name), fmt, type, num, "channels");
		snprintf(data, sizeof(data), "%lu\n", atol(value));
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set "
				"the interface audio channel enable");
			return -1;
		}
		switch (atol(value)) {
		case SDIAUDIO_CTL_AUDCH_EN_0:
			printf("\tDisabling audio.\n");
			break;
		case SDIAUDIO_CTL_AUDCH_EN_2:
			printf("\tAssuming 2 channels of audio.\n");
			break;
		case SDIAUDIO_CTL_AUDCH_EN_4:
			printf("\tAssuming 4 channels of audio.\n");
			break;
		case SDIAUDIO_CTL_AUDCH_EN_6:
			printf("\tAssuming 6 channels of audio.\n");
			break;
		case SDIAUDIO_CTL_AUDCH_EN_8:
			printf("\tAssuming 8 channels of audio.\n");
			break;
		default:
			printf("\tSet audio channel enable = %lu.\n", atol(value));
			break;
		}
	} else if (setting == SETTING_NON_AUDIO) {
		snprintf(name, sizeof(name), fmt, type, num, "non_audio");
		snprintf(data, sizeof(data), "0x%04lX\n", atol(value));
		if (util_write(name, data, sizeof(data)) < 0) {
			fprintf(stderr, "%s: ", device);
			perror("unable to set "
				"the interface non-audio");
			return -1;
		}
		switch (atol(value)) {
		case 0x0000:
			printf("\tPassing PCM audio.\n");
			break;
		case 0x00ff:
			printf("\tPassing non-audio.\n");
			break;
		default:
			printf("\tSet non-audio = 0x%04lX.\n", atol(value));
			break;
		}
	}

	return 0;
}

static ssize_t util_read(const char *name, char *buf, size_t count) {
	ssize_t fd, ret;

	if ((fd = open(name, O_RDONLY)) < 0) {
		return fd;
	}
	ret = read(fd, buf, count);
	close(fd);
	return ret;
}

static ssize_t util_write(const char *name, const char *buf, size_t count) {
	ssize_t fd, ret;

	if ((fd = open(name, O_WRONLY)) < 0) {
		return fd;
	}
	ret = write(fd, buf, count);
	close(fd);
	return ret;
}

static char * itoa(uint64_t i) {

	if (i == 0)
		return strdup("0");

	char * mystring = (char *) malloc(50);
	sprintf(mystring, "%"PRIu64, i);

	return mystring;
}
