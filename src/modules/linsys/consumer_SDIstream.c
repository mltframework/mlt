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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

#include "sdi_generator.c"

// definitions

#define MAX_AUDIO_SAMPLES (1920*2)  // 1920 Samples per Channel and we have stereo (48000 Hz /25 fps = 1920 Samples per frame)
#define PAL_HEIGHT 576				// only used for MAX_FRAMESIZE
#define PAL_WIDTH 720				// only used for MAX_FRAMESIZE
#define MAX_FRAMESIZE (PAL_HEIGHT*PAL_WIDTH*2) // SDI frame size, (2 Pixels are represented by 4 bytes, yuyv422)

// alias for "struct consumer_SDIstream_s *" , now we can write "consumer_SDIstream". Makes it more readable...
typedef struct consumer_SDIstream_s *consumer_SDIstream;

struct consumer_SDIstream_s {

	struct mlt_consumer_s parent; // This is the basic Consumer from which we fetch our data
	mlt_image_format pix_fmt; // Must be mlt_image_yuv422 for SDI
	mlt_audio_format aformat; // default: mlt_audio_pcm
	int width;
	int height;
	int frequency;
	int samples;
	int channels;
	char *path_destination_sdi; // Path for SDI output
	uint8_t video_buffer[MAX_FRAMESIZE]; // our picture for this frame
	int16_t audio_buffer0[MAX_AUDIO_SAMPLES]; // our audio channelpair 1 for this frame
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

	// Loop until told not to
	while (!consumer_is_stopped(consumer) && terminated == 0) { //

		// Get a frame from the service
		if ((frame = mlt_consumer_rt_frame(consumer)) != NULL) {

			// Check for termination
			if (terminate_on_pause && frame != NULL) {
				terminated = mlt_properties_get_double(MLT_FRAME_PROPERTIES(frame), "_speed") == 0.0;
				if (terminated == 1) {
					printf("\nEnd of playout reached, terminating\n");
					fflush(stdout);
					consumer_stop(consumer);
				}
			}

			// True if mlt_consumer_rt_frame(...) successful
			if (mlt_properties_get_int(mlt_frame_properties(frame), "rendered") == 1) {

				// Get the video from this frame and save it to our video_buffer
				mlt_frame_get_image(frame, &this->video_buffer, &this->pix_fmt, &this->width, &this->height, 1);

				// Get the audio from this frame and save it to our audio_buffer
				//mlt_frame_get_audio(frame, &this->audio_buffer0, &this->aformat, &this->frequency, &this->channels, &this->samples );
				mlt_frame_get_audio(frame, &this->audio_buffer0, &this->aformat, &this->frequency, &this->channels, &this->samples);

				//printf("\n &this->channels: %i\n", this->channels);

				// Tell the sdi_generator.c to playout our frame
				// 8 audio streams with 2 stereo channels are possible
				// copy first to all audio stream, because no second or more audio streams are currently available (we think)
				// TODO struct params with <n> audio buffer and properties (size_of)
				if (this->video_buffer) {

					my_dbn = sdimaster_playout(&this->video_buffer, &this->audio_buffer0, &this->audio_buffer0, &this->audio_buffer0, &this->audio_buffer0,
							&this->audio_buffer0, &this->audio_buffer0, &this->audio_buffer0, &this->audio_buffer0, sizeof(this->video_buffer),
							sizeof(this->audio_buffer0), 1, my_dbn);
				} else
					printf("SDI-Consumer: Videobuffer was NULL, skipping playout!\n");

			} else {
				printf("WARNING the requested frame is not yet rendered! This will cause image disturbance!\n");
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
