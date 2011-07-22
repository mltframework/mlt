/*
 * consumer_decklink.c -- output through Blackmagic Design DeckLink
 * Copyright (C) 2010 Dan Dennedy <dan@dennedy.org>
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
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#ifdef WIN32
#include <objbase.h>
#include "DeckLinkAPI_h.h"
#else
#include "DeckLinkAPI.h"
#endif

static const unsigned PREROLL_MINIMUM = 3;

class DeckLinkConsumer
	: public IDeckLinkVideoOutputCallback
{
private:
	mlt_consumer_s              m_consumer;
	IDeckLink*                  m_deckLink;
	IDeckLinkOutput*            m_deckLinkOutput;
	IDeckLinkDisplayMode*       m_displayMode;
	int                         m_width;
	int                         m_height;
	BMDTimeValue                m_duration;
	BMDTimeScale                m_timescale;
	double                      m_fps;
	uint64_t                    m_count;
	int                         m_channels;
	unsigned                    m_dropped;
	IDeckLinkMutableVideoFrame* m_decklinkFrame;
	bool                        m_isAudio;
	int                         m_isKeyer;
	IDeckLinkKeyer*             m_deckLinkKeyer;
	bool                        m_terminate_on_pause;
	uint32_t                    m_preroll;

	IDeckLinkDisplayMode* getDisplayMode()
	{
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( getConsumer() ) );
		IDeckLinkDisplayModeIterator* iter;
		IDeckLinkDisplayMode* mode;
		IDeckLinkDisplayMode* result = 0;
		
		if ( m_deckLinkOutput->GetDisplayModeIterator( &iter ) == S_OK )
		{
			while ( !result && iter->Next( &mode ) == S_OK )
			{
				m_width = mode->GetWidth();
				m_height = mode->GetHeight();
				mode->GetFrameRate( &m_duration, &m_timescale );
				m_fps = (double) m_timescale / m_duration;
				int p = mode->GetFieldDominance() == bmdProgressiveFrame;
				mlt_log_verbose( getConsumer(), "BMD mode %dx%d %.3f fps prog %d\n", m_width, m_height, m_fps, p );
				
				if ( m_width == profile->width && m_height == profile->height && p == profile->progressive
					 && m_fps == mlt_profile_fps( profile ) )
					result = mode;
			}
		}
		
		return result;
	}
	
public:
	mlt_consumer getConsumer()
		{ return &m_consumer; }
	
	~DeckLinkConsumer()
	{
		if ( m_deckLinkKeyer )
			m_deckLinkKeyer->Release();
		if ( m_deckLinkOutput )
			m_deckLinkOutput->Release();
		if ( m_deckLink )
			m_deckLink->Release();
	}
	
	bool open( unsigned card = 0 )
	{
		unsigned i = 0;
#ifdef WIN32
		IDeckLinkIterator* deckLinkIterator = NULL;
		HRESULT result =  CoInitialize( NULL );
		if ( FAILED( result ) )
		{
			mlt_log_error( getConsumer(), "COM initialization failed\n" );
			return false;
		}
		result = CoCreateInstance( CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**) &deckLinkIterator );
		if ( FAILED( result ) )
		{
			mlt_log_error( getConsumer(), "The DeckLink drivers not installed.\n" );
			return false;
		}
#else
		IDeckLinkIterator* deckLinkIterator = CreateDeckLinkIteratorInstance();
		
		if ( !deckLinkIterator )
		{
			mlt_log_error( getConsumer(), "The DeckLink drivers not installed.\n" );
			return false;
		}
#endif
		
		// Connect to the Nth DeckLink instance
		do {
			if ( deckLinkIterator->Next( &m_deckLink ) != S_OK )
			{
				mlt_log_error( getConsumer(), "DeckLink card not found\n" );
				deckLinkIterator->Release();
				return false;
			}
		} while ( ++i <= card );
		deckLinkIterator->Release();
		
		// Obtain the audio/video output interface (IDeckLinkOutput)
		if ( m_deckLink->QueryInterface( IID_IDeckLinkOutput, (void**)&m_deckLinkOutput ) != S_OK )
		{
			mlt_log_error( getConsumer(), "No DeckLink cards support output\n" );
			m_deckLink->Release();
			m_deckLink = 0;
			return false;
		}
		
		// Get the keyer interface
		IDeckLinkAttributes *deckLinkAttributes = 0;
		m_deckLinkKeyer = 0;
		if ( m_deckLink->QueryInterface( IID_IDeckLinkAttributes, (void**) &deckLinkAttributes ) == S_OK )
		{
#ifdef WIN32
			BOOL flag = FALSE;
#else
			bool flag = false;
#endif
			if ( deckLinkAttributes->GetFlag( BMDDeckLinkSupportsInternalKeying, &flag ) == S_OK && flag )
			{
				if ( m_deckLink->QueryInterface( IID_IDeckLinkKeyer, (void**) &m_deckLinkKeyer ) != S_OK )
				{
					mlt_log_error( getConsumer(), "Failed to get keyer\n" );
					m_deckLinkOutput->Release();
					m_deckLinkOutput = 0;
					m_deckLink->Release();
					m_deckLink = 0;
					return false;
				}
			}
			deckLinkAttributes->Release();
		}

		// Provide this class as a delegate to the audio and video output interfaces
		m_deckLinkOutput->SetScheduledFrameCompletionCallback( this );
		
		return true;
	}
	
	bool start( unsigned preroll )
	{
		unsigned i;
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		// Initialize members
		m_count = 0;
		m_dropped = 0;
		m_decklinkFrame = NULL;
		preroll = preroll < PREROLL_MINIMUM ? PREROLL_MINIMUM : preroll;
		m_channels = mlt_properties_get_int( properties, "channels" );
		m_isAudio = !mlt_properties_get_int( properties, "audio_off" );
		m_terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );


		m_displayMode = getDisplayMode();
		if ( !m_displayMode )
		{
			mlt_log_error( getConsumer(), "Profile is not compatible with decklink.\n" );
			return false;
		}
		
		// Set the keyer
		if ( m_deckLinkKeyer && ( m_isKeyer = mlt_properties_get_int( properties, "keyer" ) ) )
		{
			bool external = (m_isKeyer == 2);
			double level = mlt_properties_get_double( properties, "keyer_level" );

			if ( m_deckLinkKeyer->Enable( external ) != S_OK )
				mlt_log_error( getConsumer(), "Failed to enable %s keyer\n",
					external ? "external" : "internal" );
			m_deckLinkKeyer->SetLevel( level <= 1 ? ( level > 0 ? 255 * level : 255 ) : 255 );
		}
		else if ( m_deckLinkKeyer )
		{
			m_deckLinkKeyer->Disable();
		}

		// Set the video output mode
		if ( S_OK != m_deckLinkOutput->EnableVideoOutput( m_displayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) )
		{
			mlt_log_error( getConsumer(), "Failed to enable video output\n" );
			return false;
		}

		// Set the audio output mode
		if ( !m_isAudio )
		{
			m_deckLinkOutput->DisableAudioOutput();
			return true;
		}
		if ( S_OK != m_deckLinkOutput->EnableAudioOutput( bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger,
			m_channels, bmdAudioOutputStreamTimestamped ) )
		{
			mlt_log_error( getConsumer(), "Failed to enable audio output\n" );
			stop();
			return false;
		}

		m_preroll = preroll;

		// preroll frames
		for( i = 0; i < preroll; i++ )
			ScheduleNextFrame( true );

		// start scheduled playback
		m_deckLinkOutput->StartScheduledPlayback( 0, m_timescale, 1.0 );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		return true;
	}
	
	bool stop()
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		// set running state is 0
		mlt_properties_set_int( properties, "running", 0 );
		mlt_consumer_stopped( getConsumer() );

		// release decklink frame
		if ( m_decklinkFrame )
			m_decklinkFrame->Release();
		m_decklinkFrame = NULL;

		// Stop the audio and video output streams immediately
		if ( m_deckLinkOutput )
		{
			m_deckLinkOutput->StopScheduledPlayback( 0, 0, 0 );
			m_deckLinkOutput->DisableAudioOutput();
			m_deckLinkOutput->DisableVideoOutput();
		}

		return true;
	}

	void renderAudio( mlt_frame frame )
	{
		mlt_audio_format format = mlt_audio_s16;
		int frequency = bmdAudioSampleRate48kHz;
		int samples = mlt_sample_calculator( m_fps, frequency, m_count );
		int16_t *pcm = 0;

		if ( !mlt_frame_get_audio( frame, (void**) &pcm, &format, &frequency, &m_channels, &samples ) )
		{
			uint32_t written = 0;
			BMDTimeValue streamTime = m_count * frequency * m_duration / m_timescale;
			m_deckLinkOutput->GetBufferedAudioSampleFrameCount(&written);
			if ( written > (m_preroll + 1) * samples )
			{
				mlt_log_verbose( getConsumer(), "renderAudio: will flush %d audiosamples\n", written);
				m_deckLinkOutput->FlushBufferedAudioSamples();
			};
#ifdef WIN32
			m_deckLinkOutput->ScheduleAudioSamples( pcm, samples, streamTime, frequency, (unsigned long*) &written );
#else
			m_deckLinkOutput->ScheduleAudioSamples( pcm, samples, streamTime, frequency, &written );
#endif

			if ( written != (uint32_t) samples )
				mlt_log_verbose( getConsumer(), "renderAudio: samples=%d, written=%u\n", samples, written );
		}
	}

	bool createFrame( IDeckLinkMutableVideoFrame** decklinkFrame )
	{
		BMDPixelFormat format = m_isKeyer? bmdFormat8BitARGB : bmdFormat8BitYUV;
		IDeckLinkMutableVideoFrame* frame = 0;
		uint8_t *buffer = 0;
		int stride = m_width * ( m_isKeyer? 4 : 2 );

		*decklinkFrame = NULL;

		// Generate a DeckLink video frame
		if ( S_OK != m_deckLinkOutput->CreateVideoFrame( m_width, m_height,
			stride, format, bmdFrameFlagDefault, &frame ) )
		{
			mlt_log_verbose( getConsumer(), "Failed to create video frame\n" );
			stop();
			return false;
		}
		
		// Make the first line black for field order correction.
		if ( S_OK == frame->GetBytes( (void**) &buffer ) && buffer )
		{
			if ( m_isKeyer )
			{
				memset( buffer, 0, stride );
			}
			else for ( int i = 0; i < m_width; i++ )
			{
				*buffer++ = 128;
				*buffer++ = 16;
			}
		}

		*decklinkFrame = frame;

		return true;
	}

	void renderVideo( mlt_frame frame )
	{
		mlt_image_format format = m_isKeyer? mlt_image_rgb24a : mlt_image_yuv422;
		uint8_t* image = 0;
		int rendered = mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered");

		if ( rendered && !mlt_frame_get_image( frame, &image, &format, &m_width, &m_height, 0 ) )
		{
			uint8_t* buffer = 0;
			int stride = m_width * ( m_isKeyer? 4 : 2 );

			if ( m_decklinkFrame )
				m_decklinkFrame->Release();
			if ( createFrame( &m_decklinkFrame ) )
				m_decklinkFrame->GetBytes( (void**) &buffer );

			if ( buffer )
			{
				int progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "progressive" );

				if ( !m_isKeyer )
				{
					// Normal non-keyer playout - needs byte swapping
					if ( !progressive && m_displayMode->GetFieldDominance() == bmdUpperFieldFirst )
						// convert lower field first to top field first
						swab( (char*) image, (char*) buffer + stride, stride * ( m_height - 1 ) );
					else
						swab( (char*) image, (char*) buffer, stride * m_height );
				}
				else if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "test_image" ) )
				{
					// Normal keyer output
					int y = m_height + 1;
					uint32_t* s = (uint32_t*) image;
					uint32_t* d = (uint32_t*) buffer;

					if ( !progressive && m_displayMode->GetFieldDominance() == bmdUpperFieldFirst )
					{
						// Correct field order
						m_height--;
						y--;
						d += m_width;
					}

					// Need to relocate alpha channel RGBA => ARGB
					while ( --y )
					{
						int x = m_width + 1;
						while ( --x )
						{
							*d++ = ( *s << 8 ) | ( *s >> 24 );
							s++;
						}
					}
				}
				else
				{
					// Keying blank frames - nullify alpha
					memset( buffer, 0, stride * m_height );
				}
			}
		}
		if ( m_decklinkFrame )
			m_deckLinkOutput->ScheduleVideoFrame( m_decklinkFrame, m_count * m_duration, m_duration, m_timescale );

		if ( !rendered )
			mlt_log_verbose( getConsumer(), "dropped video frame %u\n", ++m_dropped );
	}

	HRESULT render( mlt_frame frame )
	{
		HRESULT result = S_OK;

		// Get the audio
		double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "_speed" );
		if ( m_isAudio && speed == 1.0 )
			renderAudio( frame );

		// Get the video
		renderVideo( frame );
		++m_count;

		return result;
	}
	
	// *** DeckLink API implementation of IDeckLinkVideoOutputCallback IDeckLinkAudioOutputCallback *** //

	// IUnknown needs only a dummy implementation
	virtual HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, LPVOID *ppv )
		{ return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef()
		{ return 1; }
	virtual ULONG STDMETHODCALLTYPE Release()
		{ return 1; }
	
	/************************* DeckLink API Delegate Methods *****************************/
	
	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult completed )
	{
		// When a video frame has been released by the API, schedule another video frame to be output

		// ignore handler if frame was flushed
		if(bmdOutputFrameFlushed == completed)
			return S_OK;

		// schedule next frame
		ScheduleNextFrame(false);

		// step forward frames counter if underrun
		if(bmdOutputFrameDisplayedLate == completed)
		{
			mlt_log_verbose( getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDisplayedLate == completed\n");
			m_count++;
		}
		if(bmdOutputFrameDropped == completed)
		{
			mlt_log_verbose( getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDropped == completed\n");
			m_count++;
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
	{
		return mlt_consumer_is_stopped( getConsumer() ) ? S_FALSE : S_OK;
	}
	

	void ScheduleNextFrame(bool preroll)
	{
		// get the consumer
		mlt_consumer consumer = getConsumer();

		// Get the properties
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

		// Frame and size
		mlt_frame frame = NULL;

		if( mlt_properties_get_int( properties, "running" ) || preroll )
		{
			frame = mlt_consumer_rt_frame( consumer );
			if ( frame != NULL )
			{
				render( frame );

				mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

				// terminate on pause
				if (m_terminate_on_pause &&
					mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0)
					stop();

				mlt_frame_close( frame );
			}
		}
	}
};

/** Start the consumer.
 */

static int start( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	DeckLinkConsumer* decklink = (DeckLinkConsumer*) consumer->child;
	return decklink->start( mlt_properties_get_int( properties, "preroll" ) ) ? 0 : 1;
}

/** Stop the consumer.
 */

static int stop( mlt_consumer consumer )
{
	// Get the properties
	DeckLinkConsumer* decklink = (DeckLinkConsumer*) consumer->child;
	return decklink->stop();
}

/** Determine if the consumer is stopped.
 */

static int is_stopped( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	return !mlt_properties_get_int( properties, "running" );
}

/** Close the consumer.
 */

static void close( mlt_consumer consumer )
{
	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the parent
	consumer->close = NULL;
	mlt_consumer_close( consumer );

	// Free the memory
	delete (DeckLinkConsumer*) consumer->child;
}

extern "C" {

/** Initialise the consumer.
 */

mlt_consumer consumer_decklink_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the consumer
	DeckLinkConsumer* decklink = new DeckLinkConsumer();
	mlt_consumer consumer = NULL;

	// If allocated
	if ( decklink && !mlt_consumer_init( decklink->getConsumer(), decklink, profile ) )
	{
		// If initialises without error
		if ( decklink->open( arg? atoi(arg) : 0 ) )
		{
			consumer = decklink->getConsumer();
			
			// Setup callbacks
			consumer->close = close;
			consumer->start = start;
			consumer->stop = stop;
			consumer->is_stopped = is_stopped;
			mlt_properties_set( MLT_CONSUMER_PROPERTIES(consumer), "deinterlace_method", "onefield" );
		}
	}

	// Return consumer
	return consumer;
}

extern mlt_producer producer_decklink_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	const char *service_type = NULL;
	switch ( type )
	{
		case consumer_type:
			service_type = "consumer";
			break;
		case producer_type:
			service_type = "producer";
			break;
		default:
			return NULL;
	}
	snprintf( file, PATH_MAX, "%s/decklink/%s_%s.yml", mlt_environment( "MLT_DATA" ), service_type, id );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "decklink", consumer_decklink_init );
	MLT_REGISTER( producer_type, "decklink", producer_decklink_init );
	MLT_REGISTER_METADATA( consumer_type, "decklink", metadata, NULL );
	MLT_REGISTER_METADATA( producer_type, "decklink", metadata, NULL );
}

} // extern C
