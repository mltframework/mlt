/*
 * filter_dynamictext.c -- dynamic text overlay filter
 * Copyright (C) 2011-2014 Meltytech, LLC
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h> // for stat()
#include <sys/stat.h>  // for stat()
#include <unistd.h>    // for stat()
#include <time.h>      // for strftime() and gtime()

#define MAX_TEXT_LEN 512

/** Get the next token and indicate whether it is enclosed in "# #".
*/
static int get_next_token(char* str, int* pos, char* token, int* is_keyword)
{
	int token_pos = 0;
	int str_len = strlen( str );

	if( (*pos) >= str_len || str[*pos] == '\0' )
	{
		return 0;
	}

	if( str[*pos] == '#' )
	{
		*is_keyword = 1;
		(*pos)++;
	}
	else
	{
		*is_keyword = 0;
	}

	while( *pos < str_len && token_pos < MAX_TEXT_LEN - 1)
	{
		if( str[*pos] == '\\' && str[(*pos) + 1] == '#' )
		{
			// Escape Sequence - "#" preceeded by "\" - copy the # into the token.
			token[token_pos] = '#';
			token_pos++;
			(*pos)++; // skip "\"
			(*pos)++; // skip "#"
		}
		else if( str[*pos] == '#' )
		{
			if( *is_keyword )
			{
				// Found the end of the keyword
				(*pos)++;
			}
			break;
		}
		else
		{
			token[token_pos] = str[*pos];
			token_pos++;
			(*pos)++;
		}
	}

	token[token_pos] = '\0';

	return 1;
}

static void get_timecode_str( mlt_filter filter, mlt_frame frame, char* text, mlt_time_format time_format )
{
	mlt_position frames = mlt_frame_get_position( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	char *s = mlt_properties_frames_to_time( properties, frames, time_format );
	if ( s )
		strncat( text, s, MAX_TEXT_LEN - strlen( text ) - 1 );
}

static void get_frame_str( mlt_filter filter, mlt_frame frame, char* text )
{
	int pos = mlt_frame_get_position( frame );
	char s[12];
	snprintf( s, sizeof( s ) - 1, "%d", pos );
	strncat( text, s, MAX_TEXT_LEN - strlen( text ) - 1 );
}

static void get_filedate_str( const char* keyword, mlt_filter filter, mlt_frame frame, char* text )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	char* filename = mlt_properties_get( producer_properties, "resource");
	struct stat file_info;

	if( !stat(filename, &file_info))
	{
		const char *format = "%Y/%m/%d";
		int n = strlen( "filedate" ) + 1;
		struct tm* time_info = gmtime( &(file_info.st_mtime) );
		char *date = calloc( 1, MAX_TEXT_LEN );

		if ( strlen( keyword ) > n )
			format = &keyword[n];
		strftime( date, MAX_TEXT_LEN, format, time_info );
		strncat( text, date, MAX_TEXT_LEN - strlen( text ) - 1);
		free( date );
	}
}

static void get_localfiledate_str( const char* keyword, mlt_filter filter, mlt_frame frame, char* text )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	char* filename = mlt_properties_get( producer_properties, "resource" );
	struct stat file_info;

	if( !stat( filename, &file_info ) )
	{
		const char *format = "%Y/%m/%d";
		int n = strlen( "localfiledate" ) + 1;
		struct tm* time_info = localtime( &(file_info.st_mtime) );
		char *date = calloc( 1, MAX_TEXT_LEN );

		if ( strlen( keyword ) > n )
			format = &keyword[n];
		strftime( date, MAX_TEXT_LEN, format, time_info );
		strncat( text, date, MAX_TEXT_LEN - strlen( text ) - 1);
		free( date );
	}
}

static void get_localtime_str( const char* keyword, char* text )
{
	const char *format = "%Y/%m/%d %H:%M:%S";
	int n = strlen( "localtime" ) + 1;
	time_t now = time( NULL );
	struct tm* time_info = localtime( &now );
	char *date = calloc( 1, MAX_TEXT_LEN );

	if ( strlen( keyword ) > n )
		format = &keyword[n];
	strftime( date, MAX_TEXT_LEN, format, time_info );
	strncat( text, date, MAX_TEXT_LEN - strlen( text ) - 1);
	free( date );
}

static void get_resource_str( mlt_filter filter, mlt_frame frame, char* text )
{
	mlt_producer producer = mlt_producer_cut_parent( mlt_frame_get_original_producer( frame ) );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	strncat( text, mlt_properties_get( producer_properties, "resource" ), MAX_TEXT_LEN - strlen( text ) - 1 );
}

/** Perform substitution for keywords that are enclosed in "# #".
*/
static void substitute_keywords(mlt_filter filter, char* result, char* value, mlt_frame frame)
{
	char keyword[MAX_TEXT_LEN] = "";
	int pos = 0;
	int is_keyword = 0;

	while ( get_next_token(value, &pos, keyword, &is_keyword) )
	{
		if(!is_keyword)
		{
			strncat( result, keyword, MAX_TEXT_LEN - strlen( result ) - 1 );
		}
		else if ( !strcmp( keyword, "timecode" ) || !strcmp( keyword, "smpte_df" ) )
		{
			get_timecode_str( filter, frame, result, mlt_time_smpte_df );
		}
		else if ( !strcmp( keyword, "smpte_ndf" ) )
		{
			get_timecode_str( filter, frame, result, mlt_time_smpte_ndf );
		}
		else if ( !strcmp( keyword, "frame" ) )
		{
			get_frame_str( filter, frame, result );
		}
		else if ( !strncmp( keyword, "filedate", 8 ) )
		{
			get_filedate_str( keyword, filter, frame, result );
		}
		else if ( !strncmp( keyword, "localfiledate", 13 ) )
		{
			get_localfiledate_str( keyword, filter, frame, result );
		}
		else if ( !strncmp( keyword, "localtime", 9 ) )
		{
			get_localtime_str( keyword, result );
		}
		else if ( !strcmp( keyword, "resource" ) )
		{
			get_resource_str( filter, frame, result );
		}
		else
		{
			// replace keyword with property value from this frame
			mlt_properties frame_properties = MLT_FRAME_PROPERTIES( frame );
			char *frame_value = mlt_properties_get( frame_properties, keyword );
			if( frame_value )
			{
				strncat( result, frame_value, MAX_TEXT_LEN - strlen(result) - 1 );
			}
		}
	}
}

static int setup_producer( mlt_filter filter, mlt_producer producer, mlt_frame frame )
{
	mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
	char* dynamic_text = mlt_properties_get( my_properties, "argument" );

	if ( !dynamic_text || !strcmp( "", dynamic_text ) )
		return 0;

	// Check for keywords in dynamic text
	if ( dynamic_text )
	{
		// Apply keyword substitution before passing the text to the filter.
		char result[MAX_TEXT_LEN] = "";
		substitute_keywords( filter, result, dynamic_text, frame );
		mlt_properties_set( producer_properties, "text", (char*)result );
	}

	// Pass the properties to the pango producer
	mlt_properties_set( producer_properties, "family", mlt_properties_get( my_properties, "family" ) );
	mlt_properties_set( producer_properties, "size", mlt_properties_get( my_properties, "size" ) );
	mlt_properties_set( producer_properties, "weight", mlt_properties_get( my_properties, "weight" ) );
	mlt_properties_set( producer_properties, "style", mlt_properties_get( my_properties, "style" ) );
	mlt_properties_set( producer_properties, "fgcolour", mlt_properties_get( my_properties, "fgcolour" ) );
	mlt_properties_set( producer_properties, "bgcolour", mlt_properties_get( my_properties, "bgcolour" ) );
	mlt_properties_set( producer_properties, "olcolour", mlt_properties_get( my_properties, "olcolour" ) );
	mlt_properties_set( producer_properties, "pad", mlt_properties_get( my_properties, "pad" ) );
	mlt_properties_set( producer_properties, "outline", mlt_properties_get( my_properties, "outline" ) );
	mlt_properties_set( producer_properties, "align", mlt_properties_get( my_properties, "halign" ) );

	return 1;
}

static void setup_transition( mlt_filter filter, mlt_transition transition )
{
	mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );
	mlt_properties transition_properties = MLT_TRANSITION_PROPERTIES( transition );

	mlt_service_lock( MLT_TRANSITION_SERVICE(transition) );
	mlt_properties_set( transition_properties, "geometry", mlt_properties_get( my_properties, "geometry" ) );
	mlt_properties_set( transition_properties, "halign", mlt_properties_get( my_properties, "halign" ) );
	mlt_properties_set( transition_properties, "valign", mlt_properties_get( my_properties, "valign" ) );
	mlt_properties_set_int( transition_properties, "out", mlt_properties_get_int( my_properties, "_out" ) );
	mlt_properties_set_int( transition_properties, "refresh", 1 );
	mlt_service_unlock( MLT_TRANSITION_SERVICE(transition) );
}


/** Get the image.
*/
static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	int error = 0;
	mlt_filter filter = mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_producer producer = mlt_properties_get_data( properties, "_producer", NULL );
	mlt_transition transition = mlt_properties_get_data( properties, "_transition", NULL );
	mlt_frame a_frame = 0;
	mlt_frame b_frame = 0;
	mlt_position position = 0;

	// Process all remaining filters first
	*format = mlt_image_yuv422;
	error = mlt_frame_get_image( frame, image, format, width, height, 0 );

	// Configure this filter
	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );
	if ( error || !setup_producer( filter, producer, frame ) ) {
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
		return error;
	}
	setup_transition( filter, transition );

	// Make sure the producer is in the correct position
	position = mlt_filter_get_position( filter, frame );
	mlt_producer_seek( producer, position );

	// Get the b frame and process with transition if successful
	if ( !error && mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &b_frame, 0 ) == 0 )
	{
		// This lock needs to also protect the producer properties from being
		// modified in setup_producer() while also being used in mlt_service_get_frame().
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

		// Create a temporary frame so the original stays in tact.
		a_frame = mlt_frame_clone( frame, 0 );

		// Get the a and b frame properties
		mlt_properties a_props = MLT_FRAME_PROPERTIES( a_frame );
		mlt_properties b_props = MLT_FRAME_PROPERTIES( b_frame );

		// Set the a_frame and b_frame to be in the same position and have same consumer requirements
		mlt_frame_set_position( a_frame, position );
		mlt_frame_set_position( b_frame, position );
		mlt_properties_set_int( b_props, "consumer_deinterlace", mlt_properties_get_int( a_props, "consumer_deinterlace" ) );

		// Apply all filters that are attached to this filter to the b frame
		mlt_service_apply_filters( MLT_FILTER_SERVICE( filter ), b_frame, 0 );

		// Process the frame
		mlt_transition_process( transition, a_frame, b_frame );

		// Get the image
		*format = mlt_image_yuv422;
		error = mlt_frame_get_image( a_frame, image, format, width, height, 1 );

		// Close the temporary frames
		mlt_frame_close( a_frame );
		mlt_frame_close( b_frame );
	}
	else
	{
		mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );
	}

	return error;
}

/** Filter processing.
*/
static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	// Get the properties of the frame
	mlt_properties properties = MLT_FRAME_PROPERTIES( frame );

	// Save the frame out point
	mlt_properties_set_int( MLT_FILTER_PROPERTIES( filter ), "_out", mlt_properties_get_int( properties, "out" ) );

	// Push the filter on to the stack
	mlt_frame_push_service( frame, filter );

	// Push the get_image on to the stack
	mlt_frame_push_get_image( frame, filter_get_image );

	return frame;
}

/** Constructor for the filter.
*/
mlt_filter filter_dynamictext_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new();
	mlt_transition transition = mlt_factory_transition( profile, "composite", NULL );
	mlt_producer producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "qtext:" );

	// Use pango if qtext is not available.
	if( !producer )
		producer = mlt_factory_producer( profile, mlt_environment( "MLT_PRODUCER" ), "pango:" );

	if( !producer )
		mlt_log_warning( MLT_FILTER_SERVICE(filter), "QT or GTK modules required for dynamic text.\n" );

	if ( filter && transition && producer )
	{
		mlt_properties my_properties = MLT_FILTER_PROPERTIES( filter );

		// Register the transition for reuse/destruction
    	mlt_properties_set_data( my_properties, "_transition", transition, 0, ( mlt_destructor )mlt_transition_close, NULL );

		// Register the producer for reuse/destruction
		mlt_properties_set_data( my_properties, "_producer", producer, 0, ( mlt_destructor )mlt_producer_close, NULL );

		// Ensure that we loop
		mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "eof", "loop" );

		// Assign default values
		mlt_properties_set( my_properties, "argument", arg ? arg: "#timecode#" );
		mlt_properties_set( my_properties, "geometry", "0%/0%:100%x100%:100" );
		mlt_properties_set( my_properties, "family", "Sans" );
		mlt_properties_set( my_properties, "size", "48" );
		mlt_properties_set( my_properties, "weight", "400" );
		mlt_properties_set( my_properties, "style", "normal" );
		mlt_properties_set( my_properties, "fgcolour", "0x000000ff" );
		mlt_properties_set( my_properties, "bgcolour", "0x00000020" );
		mlt_properties_set( my_properties, "olcolour", "0x00000000" );
		mlt_properties_set( my_properties, "pad", "0" );
		mlt_properties_set( my_properties, "halign", "left" );
		mlt_properties_set( my_properties, "valign", "top" );
		mlt_properties_set( my_properties, "outline", "0" );

		mlt_properties_set_int( my_properties, "_filter_private", 1 );

		filter->process = filter_process;
	}
	else
	{
		if( filter )
		{
			mlt_filter_close( filter );
		}

		if( transition )
		{
			mlt_transition_close( transition );
		}

		if( producer )
		{
			mlt_producer_close( producer );
		}

		filter = NULL;
	}
	return filter;
}
