/*
 * producer_vorbis.c -- vorbis producer
 * Copyright (C) 2003-2004 Ushodaya Enterprises Limited
 * Author: Charles Yates <charles.yates@pandora.be>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// MLT Header files
#include <framework/mlt_producer.h>
#include <framework/mlt_frame.h>
#include <framework/mlt_profile.h>

// vorbis Header files
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

// System header files
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Forward references.
static int producer_open( mlt_producer this, mlt_profile profile, char *file );
static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index );

/** Structure for metadata reading 
*/

typedef struct _sw_metadata sw_metadata;

struct _sw_metadata {
	char * name;
	char * content;
};

static sw_metadata *vorbis_metadata_from_str (char * str)
{
	sw_metadata * meta = NULL;
	int i;

	for (i = 0; str[i]; i++) {
		str[i] = tolower(str[i]);
		if (str[i] == '=') {
			str[i] = '\0';
			meta = malloc (sizeof (sw_metadata));
			meta->name = malloc( strlen(str) + 18 );
			sprintf(meta->name, "meta.attr.%s.markup", str);
			meta->content = strdup (&str[i+1]);
			break;
		}
	}
 	return meta;
}

/** Constructor for libvorbis.
*/

mlt_producer producer_vorbis_init( mlt_profile profile, mlt_service_type type, const char *id, char *file )
{
	mlt_producer this = NULL;

	// Check that we have a non-NULL argument
	if ( file != NULL )
	{
		// Construct the producer
		this = calloc( 1, sizeof( struct mlt_producer_s ) );

		// Initialise it
		if ( mlt_producer_init( this, NULL ) == 0 )
		{
			// Get the properties
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

			// Set the resource property (required for all producers)
			mlt_properties_set( properties, "resource", file );

			// Register our get_frame implementation
			this->get_frame = producer_get_frame;

			// Open the file
			if ( producer_open( this, profile, file ) != 0 )
			{
				// Clean up
				mlt_producer_close( this );
				this = NULL;
			}
		}
	}

	return this;
}

/** Destuctor for ogg files.
*/

static void producer_file_close( void *file )
{
	if ( file != NULL )
	{
		// Close the ogg vorbis structure
		ov_clear( file );

		// Free the memory
		free( file );
	}
}

/** Open the file.
*/

static int producer_open( mlt_producer this, mlt_profile profile, char *file )
{
	// FILE pointer for file
	FILE *input = fopen( file, "r" );

	// Error code to return
	int error = input == NULL;

	// Continue if file is open
	if ( error == 0 )
	{
		// OggVorbis file structure
		OggVorbis_File *ov = calloc( 1, sizeof( OggVorbis_File ) );

		// Attempt to open the stream
		error = ov == NULL || ov_open( input, ov, NULL, 0 ) != 0;

		// Assign to producer properties if successful
		if ( error == 0 )
		{
			// Get the properties
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

			// Assign the ov structure
			mlt_properties_set_data( properties, "ogg_vorbis_file", ov, 0, producer_file_close, NULL );

			// Read metadata
			sw_metadata * metadata = NULL;
			char **ptr = ov_comment(ov, -1)->user_comments;
			while(*ptr) {
				metadata = vorbis_metadata_from_str (*ptr);
				if (metadata != NULL)
					mlt_properties_set(properties, metadata->name, metadata->content);
				++ptr;
			}

			if ( ov_seekable( ov ) )
			{
				// Get the length of the file
    			double length = ov_time_total( ov, -1 );

				// We will treat everything with the producer fps
				double fps = mlt_profile_fps( profile );

				// Set out and length of file
				mlt_properties_set_position( properties, "out", ( length * fps ) - 1 );
				mlt_properties_set_position( properties, "length", ( length * fps ) );

				// Get the vorbis info
				vorbis_info *vi = ov_info( ov, -1 );
				mlt_properties_set_int( properties, "audio_frequency", (int) vi->rate );
				mlt_properties_set_int( properties, "audio_channels", vi->channels );

				// Set some media metadata
				mlt_properties_set_int( properties, "meta.media.nb_streams", 1 );
				mlt_properties_set_int( properties, "audio_index", 0 );
				mlt_properties_set( properties, "meta.media.0.stream.type", "audio" );
				mlt_properties_set( properties, "meta.media.0.codec.name", "vorbis" );
				mlt_properties_set( properties, "meta.media.0.codec.long_name", "Vorbis" );
			}
		}
		else
		{
			// Clean up
			free( ov );

			// Must close input file when open fails
			fclose( input );
		}
	}

	return error;
}

/** Convert a frame position to a time code.
*/

static double producer_time_of_frame( mlt_producer this, mlt_position position )
{
	return ( double )position / mlt_producer_get_fps( this );
}

/** Get the audio from a frame.
*/

static int producer_get_audio( mlt_frame frame, void **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	// Get the properties from the frame
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );

	// Obtain the frame number of this frame
	mlt_position position = mlt_properties_get_position( frame_properties, "vorbis_position" );

	// Get the producer 
	mlt_producer this = mlt_frame_pop_audio( frame );

	// Get the producer properties
	mlt_properties properties = MLT_PRODUCER_PROPERTIES( this );

	mlt_service_lock( MLT_PRODUCER_SERVICE( this ) );

	// Get the ogg vorbis file
	OggVorbis_File *ov = mlt_properties_get_data( properties, "ogg_vorbis_file", NULL );

	// Obtain the expected frame numer
	mlt_position expected = mlt_properties_get_position( properties, "audio_expected" );

	// Get the fps for this producer
	double fps = mlt_producer_get_fps( this );

	// Get the vorbis info
	vorbis_info *vi = ov_info( ov, -1 );

	// Obtain the audio buffer
	int16_t *audio_buffer = mlt_properties_get_data( properties, "audio_buffer", NULL );

	// Get amount of audio used
	int audio_used =  mlt_properties_get_int( properties, "audio_used" );

	// Number of frames to ignore (for ffwd)
	int ignore = 0;

	// Flag for paused (silence) 
	int paused = 0;

	// Check for audio buffer and create if necessary
	if ( audio_buffer == NULL )
	{
		// Allocate the audio buffer
		audio_buffer = mlt_pool_alloc( 131072 * sizeof( int16_t ) );

		// And store it on properties for reuse
		mlt_properties_set_data( properties, "audio_buffer", audio_buffer, 0, mlt_pool_release, NULL );
	}

	// Seek if necessary
	if ( position != expected )
	{
		if ( position + 1 == expected )
		{
			// We're paused - silence required
			paused = 1;
		}
		else if ( position > expected && ( position - expected ) < 250 )
		{
			// Fast forward - seeking is inefficient for small distances - just ignore following frames
			ignore = position - expected;
		}
		else
		{
			// Seek to the required position
			ov_time_seek( ov, producer_time_of_frame( this, position ) );
			expected = position;
			audio_used = 0;
		}
	}

	// Return info in frame
	*frequency = vi->rate;
	*channels = vi->channels;

	// Get the audio if required
	if ( !paused )
	{
		// Bitstream section
		int current_section;

		// Get the number of samples for the current frame
		*samples = mlt_sample_calculator( fps, *frequency, expected ++ );

		while( *samples > audio_used  )
		{
			// Read the samples
			int bytes = ov_read( ov, ( char * )( &audio_buffer[ audio_used * 2 ] ), 4096, 0, 2, 1, &current_section );

			// Break if error or eof
			if ( bytes <= 0 )
				break;

			// Increment number of samples used
			audio_used += bytes / ( sizeof( int16_t ) * *channels );

			// Handle ignore
			while ( ignore && audio_used >= *samples )
			{
				ignore --;
				audio_used -= *samples;
				memmove( audio_buffer, &audio_buffer[ *samples * *channels ], audio_used * sizeof( int16_t ) * *channels );
				*samples = mlt_sample_calculator( fps, *frequency, expected ++ );
			}
		}

		// Now handle the audio if we have enough
		if ( audio_used >= *samples )
		{
			int size = *samples * *channels * sizeof( int16_t );

			*format = mlt_audio_s16;
			*buffer = mlt_pool_alloc( size );
			memcpy( *buffer, audio_buffer, size );
			audio_used -= *samples;
			memmove( audio_buffer, &audio_buffer[ *samples * *channels ], audio_used * *channels * sizeof( int16_t ) );
			mlt_frame_set_audio( frame, *buffer, *format, size, mlt_pool_release );
		}
		else
		{
			mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
			audio_used = 0;
		}
		
		// Store the number of audio samples still available
		mlt_properties_set_int( properties, "audio_used", audio_used );
	}
	else
	{
		// Get silence and don't touch the context
		*samples = mlt_sample_calculator( fps, *frequency, position );
		mlt_frame_get_audio( frame, buffer, format, frequency, channels, samples );
	}

	// Regardless of speed, we expect to get the next frame (cos we ain't too bright)
	mlt_properties_set_position( properties, "audio_expected", position + 1 );

	mlt_service_unlock( MLT_PRODUCER_SERVICE( this ) );

	return 0;
}

/** Our get frame implementation.
*/

static int producer_get_frame( mlt_producer this, mlt_frame_ptr frame, int index )
{
	// Create an empty frame
	*frame = mlt_frame_init( MLT_PRODUCER_SERVICE( this ) );

	// Update timecode on the frame we're creating
	mlt_frame_set_position( *frame, mlt_producer_position( this ) );

	// Set the position of this producer
	mlt_properties frame_properties = MLT_FRAME_PROPERTIES( *frame );
	mlt_properties_set_position( frame_properties, "vorbis_position", mlt_producer_frame( this ) );

	// Set up the audio
	mlt_frame_push_audio( *frame, this );
	mlt_frame_push_audio( *frame, producer_get_audio );

	// Pass audio properties to the frame
	mlt_properties_pass_list( frame_properties, MLT_PRODUCER_PROPERTIES( this ), "frequency, channels" );

	// Calculate the next timecode
	mlt_producer_prepare_next( this );

	return 0;
}
