/*
 * producer_decklink.c -- input from Blackmagic Design DeckLink
 * Copyright (C) 2011 Dan Dennedy <dan@dennedy.org>
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
 * License along with consumer library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "DeckLinkAPI.h"

class DeckLinkProducer
	: public IDeckLinkInputCallback
{
private:
	mlt_producer_s   m_producer;
	IDeckLink*       m_decklink;
	IDeckLinkInput*  m_decklinkInput;
	mlt_deque        m_queue;
	pthread_mutex_t  m_mutex;
	pthread_cond_t   m_condition;
	bool             m_started;

	BMDDisplayMode getDisplayMode( mlt_profile profile )
	{
		IDeckLinkDisplayModeIterator* iter;
		IDeckLinkDisplayMode* mode;
		BMDDisplayMode result = bmdDisplayModeNotSupported;

		if ( m_decklinkInput->GetDisplayModeIterator( &iter ) == S_OK )
		{
			while ( !result && iter->Next( &mode ) == S_OK )
			{
				int width = mode->GetWidth();
				int height = mode->GetHeight();
				BMDTimeValue duration;
				BMDTimeScale timescale;
				mode->GetFrameRate( &duration, &timescale );
				double fps = (double) timescale / duration;
				int p = mode->GetFieldDominance() == bmdProgressiveFrame;
				mlt_log_verbose( getProducer(), "BMD mode %dx%d %.3f fps prog %d\n", width, height, fps, p );

				if ( width == profile->width && height == profile->height && p == profile->progressive
					 && fps == mlt_profile_fps( profile ) )
					result = mode->GetDisplayMode();
			}
		}

		return result;
	}

public:

	mlt_producer getProducer()
		{ return &m_producer; }

	~DeckLinkProducer()
	{
		if ( m_decklinkInput )
			m_decklinkInput->Release();
		if ( m_decklink )
			m_decklink->Release();
		if ( m_queue )
		{
			stop();
			mlt_deque_close( m_queue );
			pthread_mutex_destroy( &m_mutex );
			pthread_cond_destroy( &m_condition );
		}
	}

	bool open( mlt_profile profile, unsigned card =  0 )
	{
		IDeckLinkIterator* decklinkIterator = CreateDeckLinkIteratorInstance();
		try
		{
			if ( !decklinkIterator )
				throw "The DeckLink drivers are not installed.";

			// Connect to the Nth DeckLink instance
			unsigned i = 0;
			do {
				if ( decklinkIterator->Next( &m_decklink ) != S_OK )
					throw "DeckLink card not found.";
			} while ( ++i <= card );
			decklinkIterator->Release();

			// Get the input interface
			if ( m_decklink->QueryInterface( IID_IDeckLinkInput, (void**) &m_decklinkInput ) != S_OK )
				throw "No DeckLink cards support input.";

			// Provide this class as a delegate to the input callback
			m_decklinkInput->SetCallback( this );

			// Initialize other members
			pthread_mutex_init( &m_mutex, NULL );
			pthread_cond_init( &m_condition, NULL );
			m_queue = mlt_deque_init();
			m_started = false;
		}
		catch ( const char *error )
		{
			if ( decklinkIterator )
				decklinkIterator->Release();
			mlt_log_error( getProducer(), "%s\n", error );
			return false;
		}
		return true;
	}

	bool start( mlt_profile profile = 0 )
	{
		if ( m_started )
			return false;
		try
		{
			if ( !profile )
				profile = mlt_service_profile( MLT_PRODUCER_SERVICE( getProducer() ) );

			// Get the display mode
			BMDDisplayMode displayMode = getDisplayMode( profile );
			if ( displayMode == bmdDisplayModeNotSupported )
				throw "Profile is not compatible with decklink.";

			// Enable video capture
			BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
			BMDVideoInputFlags flags = bmdVideoInputFlagDefault;
			if ( S_OK != m_decklinkInput->EnableVideoInput( displayMode, pixelFormat, flags ) )
				throw "Failed to enable video capture.";

			// Enable audio capture
			BMDAudioSampleRate sampleRate = bmdAudioSampleRate48kHz;
			BMDAudioSampleType sampleType = bmdAudioSampleType16bitInteger;
			int channels = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "channels" );
			if ( S_OK != m_decklinkInput->EnableAudioInput( sampleRate, sampleType, channels ) )
				throw "Failed to enable audio capture.";

			// Start capture
			m_started = m_decklinkInput->StartStreams() == S_OK;
			if ( !m_started )
				throw "Failed to start capture.";
		}
		catch ( const char *error )
		{
			m_decklinkInput->DisableVideoInput();
			mlt_log_error( getProducer(), "%s\n", error );
			return false;
		}
		return true;
	}

	void stop()
	{
		if ( m_started )
			return;

		// Release the wait in getFrame
		pthread_mutex_lock( &m_mutex );
		pthread_cond_broadcast( &m_condition );
		pthread_mutex_unlock( &m_mutex );

		m_decklinkInput->StopStreams();

		// Cleanup queue
		pthread_mutex_lock( &m_mutex );
		while ( mlt_frame frame = (mlt_frame) mlt_deque_pop_back( m_queue ) )
			mlt_frame_close( frame );
		pthread_mutex_unlock( &m_mutex );

		m_started = false;
	}

	mlt_frame getFrame()
	{
		mlt_frame frame = NULL;

		// Wait if queue is empty
		pthread_mutex_lock( &m_mutex );
		while ( mlt_deque_count( m_queue ) < 1 )
			pthread_cond_wait( &m_condition, &m_mutex );

		// Get the first frame from the queue
		frame = ( mlt_frame ) mlt_deque_pop_front( m_queue );
		pthread_mutex_unlock( &m_mutex );

		// Set frame timestamp and properties
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( getProducer() ) );
		mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
		mlt_properties_set_int( properties, "progressive", profile->progressive );
		mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
		mlt_properties_set_int( properties, "width", profile->width );
		mlt_properties_set_int( properties, "real_width", profile->width );
		mlt_properties_set_int( properties, "height", profile->height );
		mlt_properties_set_int( properties, "real_height", profile->height );
		mlt_properties_set_int( properties, "format", mlt_image_yuv422 );
		mlt_properties_set_int( properties, "audio_frequency", 48000 );
		mlt_properties_set_int( properties, "audio_channels",
			mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "channels" ) );

		return frame;
	}

	// *** DeckLink API implementation of IDeckLinkInputCallback *** //

	// IUnknown needs only a dummy implementation
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID *ppv )
		{ return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef()
		{ return 1; }
	virtual ULONG STDMETHODCALLTYPE Release()
		{ return 1; }

	/************************* DeckLink API Delegate Methods *****************************/

	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(
			IDeckLinkVideoInputFrame* video,
			IDeckLinkAudioInputPacket* audio )
	{
		// Create mlt_frame
		mlt_frame frame = mlt_frame_init( MLT_PRODUCER_SERVICE( getProducer() ) );

		// Copy video
		if ( video )
		{
			if ( !( video->GetFlags() & bmdFrameHasNoInputSource ) )
			{
				int size = video->GetRowBytes() * video->GetHeight();
				void* image = mlt_pool_alloc( size );
				void* buffer = 0;

				video->GetBytes( &buffer );
				if ( image && buffer )
				{
					swab( buffer, image, size );
					mlt_frame_set_image( frame, (uint8_t*) image, size, mlt_pool_release );
				}
				else if ( image )
				{
					mlt_log_verbose( getProducer(), "no video\n" );
					mlt_pool_release( image );
				}
			}
			else
			{
				mlt_log_verbose( getProducer(), "no signal\n" );
				mlt_frame_close( frame );
				frame = 0;
			}
		}
		else
		{
			mlt_log_verbose( getProducer(), "no video\n" );
			mlt_frame_close( frame );
			frame = 0;
		}

		// Copy audio
		if ( frame && audio )
		{
			int channels = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "channels" );
			int size = audio->GetSampleFrameCount() * channels * sizeof(int16_t);
			mlt_audio_format format = mlt_audio_s16;
			void* pcm = mlt_pool_alloc( size );
			void* buffer = 0;

			audio->GetBytes( &buffer );
			if ( buffer )
			{
				memcpy( pcm, buffer, size );
				mlt_frame_set_audio( frame, pcm, format, size, mlt_pool_release );
				mlt_properties_set_int( MLT_FRAME_PROPERTIES(frame), "audio_samples", audio->GetSampleFrameCount() );
			}
			else
			{
				mlt_log_verbose( getProducer(), "no audio\n" );
				mlt_pool_release( pcm );
			}
		}
		else
		{
			mlt_log_verbose( getProducer(), "no audio\n" );
		}

		// Put frame in queue
		if ( frame )
		{
			int queueMax = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "buffer" );
			pthread_mutex_lock( &m_mutex );
			if ( mlt_deque_count( m_queue ) < queueMax )
			{
				mlt_deque_push_back( m_queue, frame );
				pthread_cond_broadcast( &m_condition );
			}
			else
				mlt_frame_close( frame );
			pthread_mutex_unlock( &m_mutex );
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(
			BMDVideoInputFormatChangedEvents events,
			IDeckLinkDisplayMode* mode,
			BMDDetectedVideoInputFormatFlags flags )
	{
		return S_OK;
	}
};

static int get_audio( mlt_frame frame, int16_t **buffer, mlt_audio_format *format, int *frequency, int *channels, int *samples )
{
	return mlt_frame_get_audio( frame, (void**) buffer, format, frequency, channels, samples );
}

static int get_image( mlt_frame frame, uint8_t **buffer, mlt_image_format *format, int *width, int *height, int writable )
{
	return mlt_frame_get_image( frame, buffer, format, width, height, writable );
}

static int get_frame( mlt_producer producer, mlt_frame_ptr frame, int index )
{
	DeckLinkProducer* decklink = (DeckLinkProducer*) producer->child;

	// Get the next frame from the decklink object
	*frame = decklink->getFrame();

	// Calculate the next timecode
	mlt_frame_set_position( *frame, mlt_producer_position( producer ) );
	mlt_producer_prepare_next( producer );

	// Add audio and video getters
	mlt_frame_push_audio( *frame, (void*) get_audio );
	mlt_frame_push_get_image( *frame, get_image );

	return 0;
}

static void producer_close( mlt_producer producer )
{
	producer->close = NULL;
	mlt_producer_close( producer );
	delete (DeckLinkProducer*) producer->child;
}

extern "C" {

/** Initialise the producer.
 */

mlt_producer producer_decklink_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the producer
	DeckLinkProducer* decklink = new DeckLinkProducer();
	mlt_producer producer = NULL;

	// If allocated and initializes
	if ( decklink && !mlt_producer_init( decklink->getProducer(), decklink ) )
	{
		if ( decklink->open( profile, arg? atoi( arg ) : 0 ) )
		{
			producer = decklink->getProducer();

			// Set callbacks
			producer->close = (mlt_destructor) producer_close;
			producer->get_frame = get_frame;

			// Set properties
			mlt_properties_set( MLT_PRODUCER_PROPERTIES( producer ), "resource", arg );
			mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( producer ), "channels", 2 );
			mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( producer ), "buffer", 25 );

			// Start immediately
			if ( !decklink->start( profile ) )
			{
				producer_close( producer );
				producer = NULL;
			}
		}
	}

	return producer;
}

}
