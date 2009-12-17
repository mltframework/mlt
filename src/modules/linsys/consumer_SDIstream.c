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

/*
 * consumer_SDIstream.c
 * Consumer for MLT Framework > 0.2.4 (release 0.2.4 does NOT work, profiles where not implemented)
 *
 * Example:
 * 	inigo video.dv -consumer linsys_sdi:/dev/sditx0 buffer=0;
 * 	melt video.dv -consumer linsys_sdi:/dev/sditx0 buffer=0;
 *
 */

#include <framework/mlt_frame.h>
#include <framework/mlt_consumer.h>
#include <framework/mlt_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "sdi_generator.c"

typedef struct consumer_SDIstream_s *consumer_SDIstream;

struct consumer_SDIstream_s {

	struct mlt_consumer_s parent; // This is the basic Consumer from which we fetch our data
	char *path_destination_sdi; // Path for SDI output
	int16_t audio_buffer[MAX_AUDIO_STREAMS][MAX_AUDIO_SAMPLES]; // The SDI audio channel pairs for this frame
};

/** Forward references to static functions.
 */

static int consumer_start(mlt_consumer this);
static int consumer_stop(mlt_consumer this);
static int consumer_is_stopped(mlt_consumer this);
static void consumer_close(mlt_consumer parent);
static void *consumer_thread(void *);

//*****************************************************************************************************
//*****************************************************************************************************
//*******************************************SDI Master Consumer***************************************
//*****************************************************************************************************
//*****************************************************************************************************


/**
 * This is what will be called by the factory
 * @param profile: profile name for consumer
 * @param type: unused
 * @param *id: unused
 * @param *arg: pointer to output path
 **/
mlt_consumer consumer_SDIstream_init(mlt_profile profile, mlt_service_type type, const char *id, char *arg) {

	// Create the consumer object
	consumer_SDIstream this = calloc(sizeof(struct consumer_SDIstream_s), 1);

	// If malloc and consumer init ok
	if (this != NULL && mlt_consumer_init(&this->parent, this, profile) == 0) {
		// Get the parent consumer object
		mlt_consumer parent = &this->parent;

		// We have stuff to clean up, so override the close method
		parent->close = consumer_close;

		// Set output path for SDI, default is "/dev/sditx0"
		if (arg == NULL) {
			this->path_destination_sdi = strdup("/dev/sditx0");
		} else {
			this->path_destination_sdi = strdup(arg);
		}

		// Set up start/stop/terminated callbacks
		parent->start = consumer_start;
		parent->stop = consumer_stop;
		parent->is_stopped = consumer_is_stopped;

		// Hardcode the audio sample rate to 48KHz for SDI
		mlt_properties_set_int(MLT_CONSUMER_PROPERTIES(parent), "frequency", 48000);

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
 * Stop the consumer.
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
 * Determine if the consumer is stopped.
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
	int terminate_on_pause = mlt_properties_get_int(MLT_CONSUMER_PROPERTIES(consumer), "terminate_on_pause");
	int terminated = 0;

	// Define a frame pointer
	mlt_frame frame;

	if (!sdimaster_init(this->path_destination_sdi, 0)) {
		exit(0);
	}

	// set Datablock number for SDI encoding
	int my_dbn = 1;

	double fps = mlt_properties_get_double(MLT_CONSUMER_PROPERTIES(consumer), "fps");
	unsigned int count = 0;

	// Loop until told not to
	while (!consumer_is_stopped(consumer) && terminated == 0) { //

		// Get a frame from the service
		if ((frame = mlt_consumer_rt_frame(consumer)) != NULL) {

			// Check for termination
			if (terminate_on_pause && frame != NULL) {
				terminated = mlt_properties_get_double(MLT_FRAME_PROPERTIES(frame), "_speed") == 0.0;
				if (terminated == 1) {
					mlt_log_info(MLT_CONSUMER_SERVICE(consumer), "End of playout reached, terminating\n");
					fflush(stdout);
					consumer_stop(consumer);
				}
			}

			// True if mlt_consumer_rt_frame(...) successful
			if (mlt_properties_get_int(mlt_frame_properties(frame), "rendered") == 1) {
				mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);

				// Tell the framework how we want our audio and video
				int width = mlt_properties_get_int(properties, "width");
				int height = mlt_properties_get_int(properties, "height");
				int frequency = 48000;
				int channels = 0;
				int samples = mlt_sample_calculator(fps, frequency, count++);
				mlt_image_format pix_fmt = mlt_image_yuv422;
				mlt_audio_format aformat = mlt_audio_s16;
				uint8_t *video_buffer;
				int16_t *audio_buffer_tmp; // the upstream audio buffer

				// Get the video from this frame and save it to our video_buffer
				mlt_frame_get_image(frame, &video_buffer, &pix_fmt, &width, &height, 0);

				// Get the audio from this frame and save it to our audio_buffer
				mlt_frame_get_audio(frame, (void**) &audio_buffer_tmp, &aformat, &frequency, &channels, &samples);
				mlt_log_debug(MLT_CONSUMER_SERVICE(consumer), "channels: %i samples: %i\n", channels, samples);
				
				int out_channels = channels;
				if ( mlt_properties_get( properties, "force_channels" ) )
					out_channels = mlt_properties_get_int( properties, "force_channels" );

				// Tell the sdi_generator.c to playout our frame
				// 8 audio streams with 2 stereo channels are possible
				if (video_buffer) {
					int i, j = 0;
					int map_channels, map_start;

					for (i = 0; i < MAX_AUDIO_STREAMS && j < out_channels; i++) {
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
					my_dbn = sdimaster_playout(video_buffer, this->audio_buffer, (out_channels + 1) / 2, my_dbn);
				} else
					mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "video_buffer was NULL, skipping playout\n");

			} else {
				mlt_log_warning(MLT_CONSUMER_SERVICE(consumer), "WARNING the requested frame is not yet rendered! This will cause image disturbance!\n");
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

	free(this->path_destination_sdi);

	// Now clean up the rest (the close = NULL is a bit nasty but needed for now)
	parent->close = NULL;
	mlt_consumer_close(parent);

	// Invoke the close function of the sdi_generator to close opened files used for output
	sdimaster_close();

	// Finally clean up this
	free(this);
}
