/*
 *
 * MLT SDI Consumer: request video and audio data from MLT and generate an SDI stream
 * Copyright (C) 2008 Broadcasting Center Europe S.A. All rights reserved.
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
 *
 * DESCRIPTION:
 * This software act as interface between the MLT Frameworkas as
 * MLT Consumer and the Linear Systems Ltd. SMPTE 259M-C boards.
 *
 * Linear Systems can be contacted at http://www.linsys.ca
 *
 * ---------------------------------------------------------------
 *      System : INTeL I686 64Bit
 *          OS : Linux SuSE Kernel 2.6.27.23-0.1-default
 *     Compiler: gcc  (c++)
 * ---------------------------------------------------------------
 *     Project : MLT SDI Consumer for SD
 *  Started by : Thomas Kurpick, Dipl.Inf. (FH)
 * ---------------------------------------------------------------
 * This program writes an SDI stream to the SDI driver provided
 * device files. E.g. SDI device file:
 *   /dev/sditx0         (SD)
 *   /dev/sditx1         (SD)
 *   /dev/sditx2         (SD)
 *   /dev/sditx4         (SD)
 *
 * Tested with:
 *      SDI Master™ PCI
 *  	SDI Master™ PCIe
 *      SDI Master™ FD PCIe LP
 *      SDI Master™ Quad/o PCIe LP
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <pthread.h>

// defines
#define MAX_AUDIO_SAMPLES (1920*2)    // 1920 Samples per Channel and we have stereo (48000 Hz /25 fps = 1920 Samples per frame)
#define TOTAL_SAMPLES 1728
#define ANCILLARY_DATA_SAMPLES 280
#define TOTAL_LINES 625
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

// function prototypes
static int sdimaster_init(char *outputpath, int format);
static int sdimaster_close();
static int sdimaster_playout(uint8_t *vBuffer, int16_t aBuffer[8][MAX_AUDIO_SAMPLES], int audio_streams, int my_DBN);

static int create_SDI_line(uint16_t *buf, int field, int active, uint8_t *video_buffer, int16_t audio_buffer[8][MAX_AUDIO_SAMPLES],
		int linenumber_sdiframe, int linenumber_video, int my_DBN, int16_t AudioGroupCounter, int16_t AudioGroups2Write, int audio_streams);
static int writeANC(uint16_t *p, int linenumber_sdiframe, uint16_t DID, int my_DBN, int16_t audio_buffer_A[MAX_AUDIO_SAMPLES], int16_t audio_buffer_B[MAX_AUDIO_SAMPLES],
		int16_t AudioDataPacketCounter, int16_t AudioGroups2Write, int audio_streams);
static uint16_t checker(uint16_t *DID_pointer);

static uint8_t getZBit(int sample_number);
static uint8_t getChannelStatusBit(uint16_t sample_number, uint8_t ch);
static int16_t getNumberOfAudioGroups2Write(int linenuber);

static uint8_t getDBN(int my_DBN);

static uint8_t *pack10(uint8_t *outbuf, uint16_t *inbuf, size_t count);

static int pack_AES_subframe(uint16_t *p, int8_t c, int8_t z, int8_t ch, int32_t audio_sample);

// Filehandler for sdi output
static int fh_sdi_dest;

struct SDI_atr {
	int status;
	int *fh;
	uint8_t *data;
	size_t framesize;
} SDI_atr;

// 192bit for AESChannelStatusBits
uint8_t AESChannelStatusBitArray[192]; // beta array
//uint8_t AESChannelStatusBitArray[24]; // TODO better way for 24x8bit !!!


/*!/brief Initialization of the file handlers for the Playout
 * @param *outputpath: file or SDITX device
 * @param format:  0:PAL, 1:NTSC (unused, PAL is always used)
 */
static int sdimaster_init(char *outputpath, int format) {

	// check video format (currently only PAL is available)
	if (format != 0) {
		printf("sdimaster_init get format '%i' ", format);
		return EXIT_FAILURE;
	}

	// open destination for SDI output
	if ((fh_sdi_dest = open(outputpath, O_WRONLY | O_CREAT, 0777)) == -1) {
		perror(NULL);
		printf("\ncould not open video output destination!\n");
		return EXIT_FAILURE;
	}

	// small description
	// http://www.sencore.com/newsletter/Nov05/DigAudioChannelStatusBits.htm
	// or
	// http://www.sencore.com/uploads/files/DigAudioChannelStatusBits.pdf

	// create empty AESChannelStatusBitArray
	int i = 0;
	for (i = 0; i < sizeof(AESChannelStatusBitArray) / sizeof(AESChannelStatusBitArray[0]); i++)
		AESChannelStatusBitArray[i] = 0;

	/**
	 * Professionel Format - Channel Status Bits
	 **/
	////// Byte 0 //////
	AESChannelStatusBitArray[0] = 1; // professional format

	AESChannelStatusBitArray[1] = 0; // PCM Format

	AESChannelStatusBitArray[2] = 1; // Emphasis: [100] No Emphasis
	AESChannelStatusBitArray[3] = 0; // ^
	AESChannelStatusBitArray[4] = 0; // ^

	AESChannelStatusBitArray[5] = 0; // locked

	AESChannelStatusBitArray[6] = 0; // sample frequncy Fs: [01]48kHz, [10]44kHz, [11]32kHz
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

	return 1;
}

//****************************************************************************************
//*****************************  PLAYOUT  ************************************************
//****************************************************************************************

/** Writes video and audio to specified files in SDI format
 * @param *vBuffer: Pointer to a video Buffer
 * @param aBuffer: An array of 8 audio channel pairs (2 mono channels)
 * @param audio_streams: number of audio streams which have content in aBuffer (available 0-8)
 *
 * @return current DBN (data block number of SDI frame)
 *
 */
static int sdimaster_playout(uint8_t *vBuffer, int16_t aBuffer[8][MAX_AUDIO_SAMPLES], int audio_streams, int my_DBN) {

	// Buffer for one line of SDI
	uint16_t buf[TOTAL_SAMPLES];

	// Buffer for the complete SDI frame
	//(*10/8 because we store (TOTAL_SAMPLES*TOTAL_LINES) words with 10 bit in this 8 bit array) )
	uint8_t data[TOTAL_SAMPLES * 10 / 8 * TOTAL_LINES];

	// Pointer to the start of data. This is used to fill data line by line
	uint8_t *p = data;

	// Size of the SDI frame (also size of data)
	size_t framesize = TOTAL_SAMPLES * 10 / 8 * TOTAL_LINES;

	//*******************************************************************************************
	//****************	Build the SDI frame line by line ****************************************
	//*******************************************************************************************
	//counter for the lines
	int i = 0;
	int16_t AudioGroupCounter = 0;

	/*#####################################################*/
	/*########	FIELD 1				#######################*/
	/*#####################################################*/

	// line 1-22	VERTICAL_BLANKING:23 lines				SAV 0x2ac		EAV 0x2d8
	for (i = 1; i <= 5; i++) {
		create_SDI_line(buf, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}
	for (i = 6; i <= 8; i++) {
		create_SDI_line(buf, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}
	for (i = 9; i <= 22; i++) {
		create_SDI_line(buf, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}
	// line 23-310  ACTIVE: 287 lines						SAV 0x200		EAV 0x274
	int f1counter = 1; // only odd lines
	for (i = 23; i <= 310; i++) {
		create_SDI_line(buf, FIELD_1, ACTIVE_VIDEO, vBuffer, aBuffer, i,
				f1counter, getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
		f1counter += 2;
	}
	i = 311;
	// line 311-312 VERTICAL_BLANKING: 2 lines				SAV 0x2ac		EAV 0x2d8
	create_SDI_line(buf, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);
	create_SDI_line(buf, FIELD_1, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	/*#####################################################*/
	/*########      FIELD 2        ########################*/
	/*#####################################################*/

	// line 313-336 VERTICAL_BLANKING: 23 lines				SAV 0x3b0		EAV 0x3c4
	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
			getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
	AudioGroupCounter += getNumberOfAudioGroups2Write(i++);
	p = pack10(p, buf, TOTAL_SAMPLES);

	// `getAudioGroups2Write()`=0
	for (i = 319; i <= 321; i++) {
		create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}
	for (i = 322; i <= 335; i++) {
		create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}
	// line 336-623 ACTIVE: 288 lines						SAV 0x31c		EAV 0x368
	int f2counter = 2; // only even Lines
	for (i = 336; i <= 623; i++) {

		create_SDI_line(buf, FIELD_2, ACTIVE_VIDEO, vBuffer, aBuffer, i,
				f2counter, getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
		f2counter += 2;
	}
	// line 624-625 VERTICAL_BLANKING: 2 lines				SAV 0x3b0		EAV 0x3c4
	for (i = 624; i <= 625; i++) {
		create_SDI_line(buf, FIELD_2, VERT_BLANKING, vBuffer, aBuffer, i, 0,
				getDBN(my_DBN++), AudioGroupCounter, getNumberOfAudioGroups2Write(i), audio_streams);
		AudioGroupCounter += getNumberOfAudioGroups2Write(i);
		p = pack10(p, buf, TOTAL_SAMPLES);
	}

	// sum of bytes that have already been written to file
	int bytes = 0;
	// store actual written bytes per 'write()
	int written_bytes = 0;

	/*****************************************/
	/**** WRITE BUFFER TO FILEHANDLE *********/
	/*****************************************/

	// Write the complete frame to output
	// The "while" is necessary because the sdi device file does not take the complete frame at once
	while (bytes < framesize) {
		if ((written_bytes = write(fh_sdi_dest, data + bytes, framesize - bytes)) < 0) {
			fprintf(stderr, "\nunable to write SDI.\n");
			return -1;
		}
		bytes += written_bytes;

		// Check for events of the SDI board
		unsigned int val;
		// printf ("Getting the transmitter event flags.\n");
		if (ioctl(fh_sdi_dest, SDI_IOC_TXGETEVENTS, &val) < 0) {
			// Maybe this is not an SDI device...

			// fprintf (stderr, "SDI output:");
			// perror ("unable to get the transmitter event flags");
		} else if (val) {
			if (val & SDI_EVENT_TX_BUFFER) {
				printf("SDI driver transmit buffer queue underrun "
					"detected.\n");
				sleep(5);
			}
			if (val & SDI_EVENT_TX_FIFO) {
				printf("SDI onboard transmit FIFO underrun detected.\n");

				// TODO react
			}
			if (val & SDI_EVENT_TX_DATA) {
				// printf("SDI transmit data change detected.\n");
			}
		}
	}

	return getDBN(my_DBN);
} // end sdimaster_playout()

// ****************************************************************************************
// ***************************  Create Line  **********************************************
// ****************************************************************************************

/** generate one SDI line
 * @param *buf: buffer to hold the line
 * @param field: size of the video Buffer
 * @param active: v-blank or active-video
 * @param *video_buffer: video buffer
 * @param audio_buffer: array of 8 channel pairs
 * @param line: linenumber
 * @param AudioGroupCounter: count written AudioGroup
 * @param AudioGroups2Write: number of samples to write
 * @param audio_streams: number of audio streams to integrate
 */
static int create_SDI_line(uint16_t *buf, int field, int active, uint8_t *video_buffer, int16_t audio_buffer[8][MAX_AUDIO_SAMPLES], int linenumber_sdiframe,
		int linenumber_video, int my_DBN, int16_t AudioGroupCounter, int16_t AudioGroups2Write, int audio_streams) {

	// write line with TRS(EAV) ANC(audio) TRS(SAV) activeVideo(CbY1CrY2)
	// 					*************************************************************************
	// 625 lines:		| EAV |     ANC      | SAV |		[CbY1CrY2]							|
	// 					*************************************************************************
	// 1728 SDI-words:	| 4   | 	280		 | 4   |		720+360+360=1440					|
	// 					*************************************************************************

	// points to current position in line
	uint16_t *p = buf;
	// XYZ word
	uint16_t sav, eav;

	// Set XYZ-word for each case
	switch (field) {
	case FIELD_1:
		switch (active) {
		case VERT_BLANKING:
			eav = 0x2d8; // 0xb6 << 2; // set bits for XYZ-word : [MSB:1][F][V][H][P3][P2][P1][P0][0][LSB:0]
			sav = 0x2ac; // 0xab << 2;
			break;
		case ACTIVE_VIDEO:
			eav = 0x274; // 0x9d << 2;
			sav = 0x200; // 0x80 << 2;
			break;
		default:
			return -1;
		}
		break;
	case FIELD_2:
		switch (active) {
		case VERT_BLANKING:
			eav = 0x3c4; // 0xf1 << 2;
			sav = 0x3b0; // 0xec << 2;
			break;
		case ACTIVE_VIDEO:
			eav = 0x368; // 0xda << 2;
			sav = 0x31c; // 0xc7 << 2;
			break;
		default:
			return -1;
		}
		break;
	default:
		return -1;
	}

	// #########################################################################################
	/* TRS Timing Reference Signal for EAV
	 *		[3ff]
	 *		[000]
	 *		[000]
	 *		[XYZ-Wort]
	 * */
	*p++ = 0x3ff;
	*p++ = 0x000;
	*p++ = 0x000;
	*p++ = eav;
	// #########################################################################################

	/* ANC Ancillary Data with AES
	 *
	 *	[ADF][ADF][ADF][DID][DBN][DC][UDW]...[UDW][CS]
	 *
	 * */
	// write ANC Data and get number of samples are written
	// step with `p` += to the number of written samples

	// Audio Group 1 with AES Frame 1 - 2
	if (audio_streams > 0 && audio_streams <= 2) {
		p += writeANC(p, linenumber_sdiframe, 0x2FF, my_DBN, audio_buffer[0], audio_buffer[1], AudioGroupCounter, AudioGroups2Write, audio_streams);
	}

	// Audio Group 2 with AES Frame 3 - 4
	if (audio_streams > 2 && audio_streams <= 4) {
		p += writeANC(p, linenumber_sdiframe, 0x1FD, my_DBN, audio_buffer[2], audio_buffer[3], AudioGroupCounter, AudioGroups2Write, audio_streams);
	}

	// Audio Group 3 with AES Frame 5 - 6
	if (audio_streams > 4 && audio_streams <= 6)
		p += writeANC(p, linenumber_sdiframe, 0x1FB, my_DBN, audio_buffer[4], audio_buffer[5], AudioGroupCounter, AudioGroups2Write, audio_streams);

	if (audio_streams > 6 && audio_streams <= 8)
		// Audio Group 4 with AES Frame 6 - 7
		p += writeANC(p, linenumber_sdiframe, 0x2F9, my_DBN, audio_buffer[6], audio_buffer[7], AudioGroupCounter, AudioGroups2Write, audio_streams);

	// Fill ANC data in until the end (position(p) to `ANCILLARY_DATA_SAMPLES`)
	while (p < (buf + ANCILLARY_DATA_SAMPLES + 4)) {
		// video color: black
		*p++ = 0x200;
		*p++ = 0x040;
	}
	// #########################################################################################
	// TRS Timing Reference Signal for SAV
	*p++ = 0x3ff;
	*p++ = 0x000;
	*p++ = 0x000;
	*p++ = sav;
	// #########################################################################################

	// If VBlank then fill the line with 0x200 and 0x040
	switch (active) {
	default:
	case VERT_BLANKING:
		while (p < (buf + TOTAL_SAMPLES)) {
			*p++ = 0x200;
			*p++ = 0x040;
		}
		break;
	case ACTIVE_VIDEO:

		// Insert the Video into the Line
		while (p < (buf + TOTAL_SAMPLES)) { // fill the rest of the line with active video

			// Because we skip the first line of video, it can happen that we read too far in the buffer
			if (linenumber_video >= 576)
				linenumber_video = 575;

			*p = video_buffer[(linenumber_video * 1440) + ((p - 288) - buf) + 1] << 2; // Cb
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040; // check values
			if (*(p - 1) > 0x3c0)
				*(p - 1) = 0x3c0;
			*p = video_buffer[(linenumber_video * 1440) + ((p - 288) - buf) - 1] << 2; // Y1
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040;
			if (*(p - 1) > 0x3ac)
				*(p - 1) = 0x3ac;
			*p = video_buffer[(linenumber_video * 1440) + ((p - 288) - buf) + 1] << 2; // Cr
			p++;
			if (*(p - 1) < 0x040)
				*(p - 1) = 0x040;
			if (*(p - 1) > 0x3c0)
				*(p - 1) = 0x3c0;
			*p = video_buffer[(linenumber_video * 1440) + ((p - 288) - buf) - 1] << 2; // Y2
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

static int writeANC(uint16_t *p, int videoline_sdiframe, uint16_t DID, int my_DBN, int16_t audio_buffer_A[MAX_AUDIO_SAMPLES], int16_t audio_buffer_B[MAX_AUDIO_SAMPLES], int16_t AudioGroupCounter,
		int16_t AudioGroups2Write, int audio_streams) {

	/* ANC Ancillary
	 *
	 *	[ADF][ADF][ADF][DID][DBN][DC][UDW]...[UDW][CS]
	 *
	 * */

	// save only current position for return value
	uint16_t *pp = p;
	// 16bit buffer to write temporarily 10bit word
	uint16_t buffer = 0; // set all explicit to zero, special the bit9 for parity
	// parity_counter
	int8_t parity_counter = 0;
	// var `i` for forloops
	int i = 0;

	if (AudioGroups2Write > 0) {
		// printf("\n %i\n", (audiobuffer_line/8));

		// Ancillary Data (vgl. SMPTE 291-M page 6 )

		// 3 ADF	(Ancillary Data Flag)
		*p++ = 0x000;
		*p++ = 0x3FF;
		*p++ = 0x3FF;

		// 1 DID	(Data Identification)
		// save DID for checker()
		uint16_t *DID_pointer = p;
		*p++ = DID;// (AES Audio Data, Group

		// *p++ = 0x2FF; // (AES Audio Data, Group1=0x2FF)
		// *p++ = 0x1FD;	// (AES Audio Data, Group2=0x1FD)
		// *p++ = 0x1FB;	// (AES Audio Data, Group3=0x1FB)
		// *p++ = 0x2F9;	// (AES Audio Data, Group4=0x2F9)

		// 1 DBN	(Data Block Number) inactiv: 1000000000 b9,b8,b7-b0	; SMPTE 272-M chapter15.1
		// *p++ = 0x200;

		// 1 DBN should start with previus DBN of SDI-Frame
		// 	-need "previus DBN" or "current framenumber"
		// 		SDI-LINE	DBN
		// 		[1]			[1]		<< start sdi frame >>
		// 		[2]			[2]
		// 		[.]			[.]
		// 		[255]		[255]
		// 		[256]		[1]
		// 		[257]		[2]
		// 		[.]			[.]
		// 		[510]		[255]
		// 		[511]		[1]
		// 		[512]		[2]
		// 		[.]			[.]
		// 		[625]		[115]	<< end sdi frame >>
		// 		[1]			[116]	<< start sdi frame >>
		// Accuracy of videoline_sdiframe(1 up to 625) to 8bit (1-255)
		// buffer = ((videoline_sdiframe-1) % 255)+1;
		buffer = my_DBN;
		parity_counter = 0;
		// count binary ones for parity
		for (i = 0; i < 8; i++) {
			if (buffer & (1 << i))
				parity_counter++;
		}
		if ((parity_counter % 2) == 0) { // else leave the 0
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
		if ((parity_counter%2)==0) { // else leave the 0
			buffer+= 512; // 10 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		} else {
			buffer+= 256; // 01 0000 0000 // set bit8 = even parity bit and bit9 = !bit8
		}
		*p++ = buffer;

		int16_t sample_number=0;
		int16_t counter = 0;
		// write subframes:
		// = n x 1AudioGroup
		// = n x 2 x 1AESFrame
		// = n x 2 x 2samples
		// = 4samples
		// = 4 x 3words
		while (counter < AudioGroups2Write*2) { /* 4:3 */

			// write one Audio Group with 4 x AES subframes
			// ( samples for ch01,ch02,ch03,ch04 or ch05,ch06,ch07,ch08 or ch09,ch10,ch11,ch12 or ch13,ch14,ch15,ch16)
			// and use audio_buffer_A(stereo) and audio_buffer_B(stereo)
			// `pack_AES_subframe()` write 3 ANC words (3*10bit), also 1 sample

			sample_number=(AudioGroupCounter*2)+ counter;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 1),
					getZBit(sample_number/2), 0, audio_buffer_A[sample_number]); // left
			p+=3; // step 3 words

			sample_number=(AudioGroupCounter*2)+ counter+1;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 2),
					getZBit(sample_number/2), 1, audio_buffer_A[sample_number]); // right
			p+=3;

			sample_number=(AudioGroupCounter*2)+ counter;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 3),
					getZBit(sample_number/2), 2, audio_buffer_B[sample_number]); // left
			p+=3;

			sample_number=(AudioGroupCounter*2)+ counter+1;
			pack_AES_subframe(p, getChannelStatusBit(sample_number/2, 4),
					getZBit(sample_number/2), 3, audio_buffer_B[sample_number]); // right
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

	// Wir merken uns die Adresse an der DC im Feld steht und addieren auf diese den tatsächlichen Wert von DC.
	// jetzt zeigt der merker auf das letzte Datenfeld...

	// DataCounter store
	// ´ende´ steht auf DataCounter
	uint16_t *ende = DID_pointer;
	// ´ende´ zeigt auf letztes feld
	ende += (*DID_pointer) & 0xFF; // ohne parity-Bit und ¬9-Bit

	DID_pointer++;

	// DID_pointer auf Adressen kleiner 'ende' zeigt
	while (DID_pointer <= ende) {
		cs += (*DID_pointer++) & 0x1FF; // 9 x LSB
	}

	// limit to 9Bi
	cs = cs & 0x1FF;

	// bit10 NOT bit9
	// - cs invertieren (mit ~)
	// - & mit bitmaske '01 0000 0000'
	// - das übrig gebliebene Bit um eins nach links shiften
	// - auf cs addieren
	cs += ((~cs) & 0x100) << 1;

	return cs;
} // end checker


/** pack 16bit in AES subframe with 3words (30bit) and write in ´*p´
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
 * */
static int pack_AES_subframe(uint16_t *p, int8_t c, int8_t z, int8_t ch, int32_t audio_sample) {

	// push 16bit up to 20bit(32bit)
	audio_sample <<= 4; // Shift by 4 (louder)

	// parity_counter
	int8_t parity_counter = 0;

	// 16bit buffer to write 10bit of [X]word,[X+1]word,[X+2]word,
	uint16_t buffer = 0;

	// #########################################################
	// ### WORD X  ############################################
	// #########################################################
	// word X: !bit8, a5, a4, a3, a2, a1, a0, ch1, ch0, z
	// SMPTE 272M s.7
	buffer = z; // z bit every 192bit = 1
	buffer += ch << 1; // ch1 - ch0
	buffer += (audio_sample & 0x3f) << 3; // a5 - a0
	buffer += ((~buffer) & 0x100) << 1; // !bit8

	// write word ´X´
	*p++ = buffer;

	// count ones (zähle Einsen)
	int i = 0;
	for (i = 0; i < 9; i++) {
		if (buffer & 1 << i)
			parity_counter++;
	}

	// #########################################################
	// ### WORD X+1 ############################################
	// #########################################################
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

	// #########################################################
	// ### WORD X+2 ############################################
	// #########################################################
	// word X+2: !bit8, P, C, U, V, a19, a18, a17, a16, a15
	// SMPTE 272M s.7
	buffer = 0;
	buffer += (audio_sample >> 15) & 0x01F; // a15 - a19
	// default of [V][U][C] bits = `0`
	// buffer += 1<<5;	// V (AES sample validity bit)
	// buffer += 1<<6;	// U (AES user bit)
	// buffer += 1<<7;	// C (AES audio channel status bit)
	buffer += c << 7;	// C (AES audio channel status bit)

	// count ones
	for (i = 0; i < 8; i++) {
		if (buffer & 1 << i)
			parity_counter++;
	}

	// 	if (!parity_counter%2) // else leave the 0
	// 		buffer+= 1 << 8; // P (AES even parity bit)
	//
	// 	buffer += ((~buffer) & 0x100 )<<1; // !bit8
	if ((parity_counter % 2) == 0) { // else leave the 0
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
	// sample_number+=192-18;

	if (sample_number % 192 == 0) {
		// printf("1 %i\n", sample_number);
		return 1;
	} else {
		// printf("0");
		return 0;
	}
}

static uint8_t getChannelStatusBit(uint16_t sample_number, uint8_t ch) {

	// return value
	uint8_t AESChannelStatusBit = 0;

	// start in SDI line 6 also 18samples later
	// AESChannelStatusBit=((sample_number+192-18)%192);
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

	// 	// `4:3`-distribution
	// 	if(linenumber<=315){
	// 		if(linenumber>=6 && linenumber<=8){
	// 			return 0;
	// 		}
	// 		if((linenumber+5)%10==0){
	// 			return 4;
	// 		}else{
	// 			return 3;
	// 		}
	// 	}else{
	// 		if(linenumber>=319 && linenumber<=321){
	// 			return 0;
	// 		}
	// 		if((linenumber-8)%10==0){
	// 			return 4;
	// 		}else{
	// 			return 3;
	// 		}
	// 	}

	// 	// full-distribution
	// 	if(linenumber<=45){
	// 		return 4;
	// 	}else{
	// 		return 3;
	// 	}

	// 	// fullhalf-distribution
	// 	if (linenumber==625)
	// 		return 4;
	//
	// 	if (linenumber%14==0) {
	// 		return 4;
	// 	} else {
	// 		return 3;
	// 	}

}
static uint8_t getDBN(int my_DBN) {

	return ((my_DBN - 1) % 255) + 1;
}


/**
 * pack10 - pack a line of 10-bit data
 * @outbuf: pointer to the output buffer
 * @inbuf: pointer to the input buffer
 * @count: number of elements in the buffer
 *
 * Returns a pointer to the next output location.
 **/
static uint8_t * pack10(uint8_t *outbuf, uint16_t *inbuf, size_t count) {
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

// Clean up
static int sdimaster_close() {

	close(fh_sdi_dest);

	return 1;
}
