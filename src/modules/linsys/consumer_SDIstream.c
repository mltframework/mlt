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
 * and
 * Dan Dennedy                        10.Feb.2010
 * Ver. 2.0
 * See also Git commit log.
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
 * 	melt video.dv -consumer sdi:/dev/sditx0
 * 	melt video.dv -consumer sdi:/dev/sditx0 blanking=true
 * 	melt video.dv -consumer sdi dev_video=/dev/sditx0 blanking=true
 * 	melt video.dv audio_index=all -consumer sdi dev_video=/dev/sditx0 blanking=true
 *
 * SDI boards without full frame stream (without blanking):
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=1   -consumer sdi dev_video=/dev/sdivideotx0 dev_audio=/dev/sdiaudiotx0 blanking=false
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=all -consumer sdi dev_video=/dev/sdivideotx0 dev_audio=/dev/sdiaudiotx0 blanking=false
 *  melt -profile atsc_1080i_50 video.mpeg audio_index=all -consumer sdi dev_video=/dev/sdivideotx0 dev_audio=/dev/sdiaudiotx0 blanking=false jpeg_files=25 save_jpegs=channel_04.jpg
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
 * SD NTSC		720 × 486/59.94/I		interlaced			30000/1001 ~ 29.97 HZ	8 x AES (16 channels)	sdi_486i_5994	TODO:180,145,159,107
 * SD NTSC		720 × 486/59.94/I		interlaced			30000/1001 ~ 29.97 HZ	4 x AES (8 channels)	sdi_486i_5994	193
 *
 **/

#include <framework/mlt_consumer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>
#include <framework/mlt_log.h>
#include <framework/mlt_events.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WITH_JPEG
// for JPEG output
#include <jpeglib.h>
#endif

#include "sdi_generator.c"

// alias for "struct consumer_SDIstream_s *" , now we can write "consumer_SDIstream". Makes it more readable...
typedef struct consumer_SDIstream_s *consumer_SDIstream;

struct consumer_SDIstream_s {
	// Most of these values are set to their defaults by the parent consumer
	struct mlt_consumer_s parent; // This is the basic Consumer from which we fetch our data
	mlt_image_format pix_fmt; // Must be mlt_image_yuv422 for SDI

	int width;
	int height;

	struct audio_format audio_format;
	/**
	 * device file:
	 *		/dev/sditx0
	 *		/dev/sdivideotx0
	 *		/dev/sdiaudiotx0
	 **/
	char *device_file_video; // Path for SDI output
	char *device_file_audio; // Path for exlusive SDI audio output

	/**
	 *  write own HANC (ancillary data) is available for:
	 * SDI board ASSY 193 HD        'blanking=false'
	 * SDI board ASSY 180 SD quad   'blanking=true'
	 * SDI board ASSY 145 SD single 'blanking=true'
	 *
	 * 0=false, 1=true
	 *
	 **/
	uint8_t blanking;

	// our audio channel pair for this frame
	int16_t audio_buffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES]; // The SDI audio channel pairs for this frame

	char *video_fmt_name; // 1080i25, 1080p25, 576i50, 486i2997, ...

};

/**
 * Forward references to static functions.
 **/
static int consumer_start(mlt_consumer this);
static int consumer_stop(mlt_consumer this);
static int consumer_is_stopped(mlt_consumer this);
static void consumer_close(mlt_consumer parent);
static void *consumer_thread(void *);

static void consumer_write_JPEG(char * path, uint8_t **vBuffer, mlt_profile myProfile);
int convertYCBCRtoRGB(int y1, int cb, int cr, int y2, uint8_t * target_rgb);

/*****************************************************************************************************
 ****************************************** SDI Master Consumer **************************************
 *****************************************************************************************************/

/** This is what will be called by the factory
 * @param profile: profile name for consumer
 * @param type: unused
 * @param *id: unused
 * @param *arg: pointer to output path
 **/
mlt_consumer consumer_SDIstream_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg) {

	// Create the consumer object
	consumer_SDIstream this = calloc( 1, sizeof(struct consumer_SDIstream_s) );

	// If malloc and consumer init ok
	if (this != NULL && mlt_consumer_init(&this->parent, this, profile) == 0) {

		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// Set output path for SDI, default is "/dev/sditx0"
		if (arg == NULL) {
			this->device_file_video = strdup("/dev/sditx0");
		} else {
			this->device_file_video = strdup(arg);
		}

		// Set up start/stop/terminated callbacks
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// Set explicit to zero or other value
		int i, j;
		for (i = 0; i < MAX_AUDIO_STREAMS; i++) {
			for (j = 0; j < MAX_AUDIO_SAMPLES; j++) {
				this->audio_buffer[i][j] = j;
			}
		}
		
		mlt_events_register( MLT_CONSUMER_PROPERTIES(parent), "consumer-fatal-error", NULL );

		// Return the consumer produced
		return parent;
	}

	// malloc or consumer init failed
	free(this);

	// Indicate failure
	return NULL;
}

/**
 * Start the consumer.
 **/
static int consumer_start(mlt_consumer parent) {

	// Get the properties
	mlt_properties properties = mlt_consumer_properties(parent);

	// Get the actual object
	consumer_SDIstream this = parent->child;

	// Check that we're not already running
	if (!mlt_properties_get_int(properties, "running")) {
		// Allocate threads
		pthread_t *consumer_pthread = calloc(1, sizeof(pthread_t));

		// Assign the thread to properties
		mlt_properties_set_data(properties, "consumer_pthread", consumer_pthread, sizeof(pthread_t), free, NULL);

		// Set the running state
		mlt_properties_set_int(properties, "running", 1);

		// Create the the threads
		pthread_create(consumer_pthread, NULL, consumer_thread, this);
	}
	return 0;
}

/**
 * Stop the consumer
 **/
static int consumer_stop(mlt_consumer parent) {

	// Get the properties
	mlt_properties properties = mlt_consumer_properties(parent);

	// Check that we're running
	if (mlt_properties_get_int(properties, "running")) {

		// Get the threads
		pthread_t *consumer_pthread = mlt_properties_get_data(properties, "consumer_pthread", NULL);

		// Stop the threads
		mlt_properties_set_int(properties, "running", 0);

		// Wait for termination
		pthread_join(*consumer_pthread, NULL);

	}
	return 0;
}

/**
 * Determine if the consumer is stopped
 **/
static int consumer_is_stopped(mlt_consumer this) {
	// Get the properties
	mlt_properties properties = mlt_consumer_properties(this);
	return !mlt_properties_get_int(properties, "running");
}

/**
 * Threaded wrapper for pipe.
 **/
static void *consumer_thread(void *arg) {

	// Identify the arg
	consumer_SDIstream this = arg;

	// Get the consumer
	mlt_consumer consumer = &this->parent;

	// Convenience functionality (this is to stop melt/inigo after the end of a playout)
	int terminate_on_pause = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES( consumer ), "terminate_on_pause");
	int terminated = 0; // save ony status

	int save_jpegs = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES( consumer ), "save_jpegs");
	char * jpeg_folder = mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "jpeg_file");

	// If no folder is specified, skip jpeg export
	if (jpeg_folder == NULL) {
		save_jpegs = 0;
	}

	if (save_jpegs > 0)
		mlt_log_info(MLT_CONSUMER_SERVICE(consumer), "Saving a JPEG every %i frame.\n", save_jpegs);

	int counter = 0; // each second we save a Jpeg

	// set properties (path) for device files
	if (mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "dev_video") != NULL) {
		this->device_file_video = strdup(mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "dev_video"));
	}
	if (mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "dev_audio") != NULL) {
		if (this->blanking == 0) {
			this->device_file_audio = strdup(mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "dev_audio"));
		} else {
			// if we write HANC we do not write further audio data
			mlt_log_info(MLT_CONSUMER_SERVICE(consumer), "Audio device file is set but will not be used.\n");
		}
	}

	// Set additional device file defaults
	struct stat st;
	int fd = -1;
	if (this->device_file_video)
		fd = stat(this->device_file_video, &st);
	if (fd == -1) {
		free(this->device_file_video);
		this->device_file_video = strdup("/dev/sdivideotx0");
	} else {
		close(fd);
	}
	if (this->device_file_audio) {
		fd = stat(this->device_file_audio, &st);
		if (fd == -1) {
			free(this->device_file_audio);
			this->device_file_audio = strdup("/dev/sdiaudiotx0");
		} else {
			close(fd);
		}
	} else if (this->device_file_video &&
			strstr(this->device_file_video, "sdivideotx")) {
		free(this->device_file_audio);
		this->device_file_audio = strdup("/dev/sdiaudiotx0");
	}

	// set blanking flag; is not nessary we write no own blanking(HANC) for HD board ASSY 193
	if (mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "blanking")) {
		// set value
		if (!strcmp( mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "blanking"), "false")) {
			this->blanking = 0;
		} else if (!strcmp( mlt_properties_get(MLT_CONSUMER_PROPERTIES( consumer ), "blanking"), "true")) {
			this->blanking = 1;
		} else {
			this->blanking = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES( consumer ), "blanking");
		}
	} else if (this->device_file_video && strstr(this->device_file_video, "sdivideotx")) {
		this->blanking = 0;
	} else {
		// set default value without HD board, also with blanking
		this->blanking = 1;
	}

	// Define a frame pointer
	mlt_frame frame;

	// set Datablock number for SDI encoding
	int my_dbn = 1;

	double fps = mlt_properties_get_double(MLT_CONSUMER_PROPERTIES(consumer), "fps");
	unsigned int count = 0;

	// Tell the framework how we want our audio and video
	int frequency = this->audio_format.sample_rate;
	int channels = mlt_properties_get_int(	MLT_CONSUMER_PROPERTIES(consumer), "channels" );
	int samples;

	// set number of audio channels, linsys vidport model 193 is limited to 8 channels (4AES frames)
	this->audio_format.channels = 8; /* 0,2,4,6,8 */
	this->audio_format.aformat = mlt_audio_s16; /* 16, 24, 32 */
	this->audio_format.sample_rate = 48000;
	this->pix_fmt = mlt_image_yuv422;

	if (this->device_file_video && this->device_file_audio &&
		!sdi_init(this->device_file_video, this->device_file_audio, this->blanking, mlt_service_profile((mlt_service) consumer), &this->audio_format)) {
		mlt_log_fatal( MLT_CONSUMER_SERVICE(consumer), "failed to initialize\n" );
		mlt_events_fire( MLT_CONSUMER_PROPERTIES(consumer), "consumer-fatal-error", NULL );
	}

	uint8_t *video_buffer = NULL;
	int16_t *audio_buffer_tmp; // the upstream audio buffer

	// Loop until told not to
	while (!consumer_is_stopped(consumer) && terminated == 0) { //

		// Get a frame from the service
		if ((frame = mlt_consumer_rt_frame(consumer)) != NULL) {

			// Check for termination
			if (terminate_on_pause && frame != NULL) {
				terminated = mlt_properties_get_double(MLT_FRAME_PROPERTIES( frame ), "_speed") == 0.0;
				if (terminated == 1) {
					mlt_log_verbose(MLT_CONSUMER_SERVICE(consumer), "\nEnd of playout reached, terminating\n");
					consumer_stop(consumer);
				}
			}

			// True if mlt_consumer_rt_frame(...) successful
			if (mlt_properties_get_int(mlt_frame_properties(frame), "rendered") == 1) {

				// Get the video from this frame and save it to our video_buffer
				mlt_frame_get_image(frame, &video_buffer, &this->pix_fmt, &this->width, &this->height, 1);

				// Get the audio from this frame and save it to our audio_buffer
				samples = mlt_sample_calculator(fps, frequency, count++);
				mlt_frame_get_audio(frame, (void**) &audio_buffer_tmp, &this->audio_format.aformat, &frequency, &channels, &samples);

				this->audio_format.sample_rate = frequency;
				this->audio_format.samples = samples;

				/* TODO: Audio is currently hard coded to 8 channels because write 8 channels to the sdi board.
				 The Linys SDI board has to be configured with the same number of channels!
				 this->audio_format.channels = channels; // take given number of channels
				 */

				/* Tell the sdi_generator.c to playout our frame
				 * 8 AES (8 x stereo channels are possible, max. 16 channels) Linsys SD board model:	107, 159, 145, 180
				 * 4 AES (4 x stereo channels are possible, max. 8 channels) Linsys HD board model:		193
				 */
				if (video_buffer) {

					// provide mapping of audio channels
					int i, j = 0;
					int map_channels, map_start;

					for (i = 0; i < MAX_AUDIO_STREAMS && j < channels; i++) {
						char key[27];
						int c;

						sprintf(key, "meta.map.audio.%d.channels", i);
						map_channels = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), key);
						sprintf(key, "meta.map.audio.%d.start", i);
						map_start = mlt_properties_get_int(MLT_FRAME_PROPERTIES(frame), key);

						if (!map_channels)
							map_channels = channels - j;
						for (c = 0; c < map_channels && j < channels; c++, j++) {
							int16_t *src = audio_buffer_tmp + j;
							int16_t *dest = this->audio_buffer[(map_start + c) / 2] + (map_start + c) % 2;
							int s = samples + 1;

							while (--s) {
								*dest = *src;
								dest += 2;
								src += channels;
							}
						}
					}

					// generate SDI frame and playout
					my_dbn = sdi_playout(video_buffer, this->audio_buffer, &this->audio_format, (channels + 1) / 2, my_dbn);

					// write a JPEG of every X-th frame
					if (save_jpegs > 0 && counter >= save_jpegs) {
						consumer_write_JPEG(jpeg_folder, &video_buffer, mlt_service_profile((mlt_service) consumer));
						counter = 0;
					} else if (save_jpegs > 0) {
						counter++;
					}
					mlt_events_fire(MLT_CONSUMER_PROPERTIES( consumer ), "consumer-frame-show", frame, NULL );
				} else {
					mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "Videobuffer was NULL, skipping playout!\n");
				}
			} else {
				mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "WARNING the requested frame is not yet rendered! This will cause image disturbance!\n");

				if (video_buffer) {
					my_dbn = sdi_playout(video_buffer, this->audio_buffer, &this->audio_format, (channels + 1) / 2, my_dbn);
				} else {
					mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "Videobuffer was NULL, skipping playout!\n");
				}
			}

			if (frame != NULL)
				mlt_frame_close(frame);
		}

	}
	return NULL;
}

/**
 * Callback to allow override of the close method.
 **/
static void consumer_close(mlt_consumer parent) {

	// Get the actual object
	consumer_SDIstream this = parent->child;

	free(this->device_file_video);
	free(this->device_file_audio);

	// Now clean up the rest (the close = NULL is a bit nasty but needed for now)
	parent->close = NULL;
	mlt_consumer_close(parent);

	// Invoke the close function of the sdi_generator to close opened files used for output
	sdimaster_close();

	// Finally clean up this
	free(this);
}

/**
 * Write videobuffer as JPEG to path
 * @param path
 **/
static void consumer_write_JPEG(char * filename, uint8_t **vBuffer, mlt_profile myProfile) {

#ifdef WITH_JPEG

	int bytes_per_pixel = 3; // or 1 for GRACYSCALE images
	int color_space = JCS_RGB; // or JCS_GRAYSCALE for grayscale images

	uint8_t * buffer_position = *vBuffer;
	uint8_t image_rgb[myProfile->width * myProfile->height * bytes_per_pixel];

	//convert vBuffer to RGB
	int i;
	for (i = 0; i < sizeof(image_rgb) / 6; i++) {
		int y1 = *(buffer_position++);
		int cb = *(buffer_position++);
		int y2 = *(buffer_position++);
		int cr = *(buffer_position++);
		convertYCBCRtoRGB(y1, cb, cr, y2, &image_rgb[i * 6]);
	}

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;

	// this is a pointer to one row of image data
	JSAMPROW row_pointer[1];

	FILE *outfile = fopen(filename, "wb");

	if (!outfile) {
		mlt_log_error(NULL, "%s: Error opening output jpeg file %s\n!", __FILE__, filename);
		return;
	}
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);

	// Setting the parameters of the output file here
	cinfo.image_width = myProfile->width;
	cinfo.image_height = myProfile->height;
	cinfo.input_components = bytes_per_pixel;
	cinfo.in_color_space = (J_COLOR_SPACE) color_space;

	// default compression parameters, we shouldn't be worried about these
	jpeg_set_defaults(&cinfo);

	// Now do the compression
	jpeg_start_compress(&cinfo, TRUE);

	// like reading a file, this time write one row at a time
	while (cinfo.next_scanline < cinfo.image_height) {
		row_pointer[0] = &image_rgb[cinfo.next_scanline * cinfo.image_width * cinfo.input_components];
		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}
	// similar to read file, clean up after we're done compressing
	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
	fclose(outfile);
#endif
}

/**
 * converts  YCbCr samples to two 32-bit RGB values
 * @param y1 cb cr and y2 values
 * @param target pointer
 * @return 0 upon success
 **/
int convertYCBCRtoRGB(int y1, int cb, int cr, int y2, uint8_t * target_rgb) {

#ifdef WITH_JPEG

	if(y1 > 235)
		y1 = 235;
	if(y1 < 16)
		y1 = 16;

	if(y2 > 235)
		y2 = 235;
	if(y2 < 16)
		y2 = 16;

	if(cr > 240)
		cr = 240;
	if(cr < 16)
		cr = 16;

	if(cb > 240)
		cb = 240;
	if(cb < 16)
		cb = 16;

	uint8_t r1, g1, b1, r2, g2, b2;

	//pointer to current output buffer position
	uint8_t * target_pointer = target_rgb;

	r1 = y1 + 1.402 * (cr - 128);
	g1 = y1 - 0.34414 * (cb - 128) - 0.71414 * (cr - 128);
	b1 = y1 + 1.772 * (cb - 128);

	r2 = y2 + 1.402 * (cr - 128);
	g2 = y2 - 0.34414 * (cb - 128) - 0.71414 *(cr - 128);
	b2 = y2 + 1.772 * (cb - 128);

	*target_pointer++ = r1;
	*target_pointer++ = g1;
	*target_pointer++ = b1;

	*target_pointer++ = r2;
	*target_pointer++ = g2;
	*target_pointer++ = b2;

	return 0;
#endif
	return 1;
}
