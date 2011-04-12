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
#include "DeckLinkAPI.h"

static const unsigned PREROLL_MINIMUM = 3;

typedef struct
{
	int16_t *buffer;
	int size;
	int used;
	pthread_mutex_t mutex;
} *sample_fifo;

static sample_fifo sample_fifo_init()
{
	sample_fifo fifo = (sample_fifo) calloc( 1, sizeof( *fifo ) );
	pthread_mutex_init( &fifo->mutex, NULL );
	return fifo;
}

static void sample_fifo_append( sample_fifo fifo, int16_t *samples, int count )
{
	pthread_mutex_lock( &fifo->mutex );
	if ( ( fifo->size - fifo->used ) < count )
	{
		fifo->size += count * 5;
		fifo->buffer = (int16_t*) realloc( fifo->buffer, fifo->size * sizeof( int16_t ) );
	}
	memcpy( fifo->buffer + fifo->used, samples, count * sizeof( int16_t ) );
	fifo->used += count;
	pthread_mutex_unlock( &fifo->mutex );
}

static void sample_fifo_remove( sample_fifo fifo, int count )
{
	pthread_mutex_lock( &fifo->mutex );
	if ( count > fifo->used )
		count = fifo->used;
	fifo->used -= count;
	memmove( fifo->buffer, fifo->buffer + count, fifo->used * sizeof( int16_t ) );
	pthread_mutex_unlock( &fifo->mutex );
}

static void sample_fifo_close( sample_fifo fifo )
{
	free( fifo->buffer );
	pthread_mutex_destroy( &fifo->mutex );
	free( fifo );
}


class DeckLinkConsumer
	: public IDeckLinkVideoOutputCallback
	, public IDeckLinkAudioOutputCallback
{
private:
	mlt_consumer_s              m_consumer;
	IDeckLink*                  m_deckLink;
	IDeckLinkOutput*            m_deckLinkOutput;
	IDeckLinkDisplayMode*       m_displayMode;
	pthread_mutex_t             m_mutex;
	pthread_cond_t              m_condition;
	int                         m_width;
	int                         m_height;
	BMDTimeValue                m_duration;
	BMDTimeScale                m_timescale;
	double                      m_fps;
	uint64_t                    m_count;
	sample_fifo                 m_fifo;
	unsigned                    m_preroll;
	bool                        m_isPrerolling;
	unsigned                    m_prerollCounter;
	int                         m_channels;
	uint32_t                    m_maxAudioBuffer;
	mlt_deque                   m_videoFrameQ;
	mlt_frame                   m_frame;
	unsigned                    m_dropped;
	bool                        m_isAudio;
	bool                        m_isKeyer;
	IDeckLinkKeyer*             m_deckLinkKeyer;

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
	uint64_t isBuffering() const
		{ return m_prerollCounter < m_preroll; }
	
	~DeckLinkConsumer()
	{
		if ( m_deckLinkKeyer )
			m_deckLinkKeyer->Release();
		if ( m_deckLinkOutput )
			m_deckLinkOutput->Release();
		if ( m_deckLink )
			m_deckLink->Release();
		if ( m_videoFrameQ )
		{
			mlt_deque_close( m_videoFrameQ );
			pthread_mutex_destroy( &m_mutex );
			pthread_cond_destroy( &m_condition );
		}
	}
	
	bool open( unsigned card = 0 )
	{
		IDeckLinkIterator* deckLinkIterator = CreateDeckLinkIteratorInstance();
		unsigned i = 0;
		
		if ( !deckLinkIterator )
		{
			mlt_log_error( getConsumer(), "The DeckLink drivers not installed.\n" );
			return false;
		}
		
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
			bool flag = false;
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
		m_deckLinkOutput->SetAudioCallback( this );
		
		pthread_mutex_init( &m_mutex, NULL );
		pthread_cond_init( &m_condition, NULL );
		m_maxAudioBuffer = bmdAudioSampleRate48kHz;
		m_videoFrameQ = mlt_deque_init();
		
		return true;
	}
	
	bool start( unsigned preroll )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		// Initialize members
		m_count = 0;
		m_frame = 0;
		m_dropped = 0;
		m_isPrerolling = true;
		m_prerollCounter = 0;
		m_preroll = preroll < PREROLL_MINIMUM ? PREROLL_MINIMUM : preroll;
		m_channels = mlt_properties_get_int( properties, "channels" );
		m_isAudio = !mlt_properties_get_int( properties, "audio_off" );

		m_displayMode = getDisplayMode();
		if ( !m_displayMode )
		{
			mlt_log_error( getConsumer(), "Profile is not compatible with decklink.\n" );
			return false;
		}
		
		// Set the keyer
		if ( m_deckLinkKeyer && ( m_isKeyer = mlt_properties_get_int( properties, "keyer" ) ) )
		{
			bool external = false;
			double level = mlt_properties_get_double( properties, "keyer_level" );

			if ( m_deckLinkKeyer->Enable( external ) != S_OK )
				mlt_log_error( getConsumer(), "Failed to enable keyer\n" );
			m_deckLinkKeyer->SetLevel( level <= 1 ? ( level > 0 ? 255 * level : 255 ) : 255 );
			m_preroll = 0;
			m_isAudio = false;
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
			m_channels, bmdAudioOutputStreamContinuous ) )
		{
			mlt_log_error( getConsumer(), "Failed to enable audio output\n" );
			stop();
			return false;
		}
		m_fifo = sample_fifo_init();
		m_deckLinkOutput->BeginAudioPreroll();
		
		return true;
	}
	
	void wakeup()
	{
		pthread_mutex_lock( &m_mutex );
		pthread_cond_broadcast( &m_condition );
		pthread_mutex_unlock( &m_mutex );
	}
	
	void wait()
	{
		struct timeval tv;
		struct timespec ts;
		
		gettimeofday( &tv, NULL );
		ts.tv_sec = tv.tv_sec + 1;
		ts.tv_nsec = tv.tv_usec * 1000;
		pthread_mutex_lock( &m_mutex );
		pthread_cond_timedwait( &m_condition, &m_mutex, &ts );
		pthread_mutex_unlock( &m_mutex );
	}
	
	void stop()
	{
		// Stop the audio and video output streams immediately
		if ( m_deckLinkOutput )
		{
			m_deckLinkOutput->StopScheduledPlayback( 0, 0, 0 );
			m_deckLinkOutput->DisableAudioOutput();
			m_deckLinkOutput->DisableVideoOutput();
		}
		while ( mlt_deque_count( m_videoFrameQ ) )
		{
			IDeckLinkMutableVideoFrame* frame = (IDeckLinkMutableVideoFrame*) mlt_deque_pop_back( m_videoFrameQ );
			frame->Release();
		}
		if ( m_fifo ) sample_fifo_close( m_fifo );
		mlt_frame_close( m_frame );
	}

	void renderAudio( mlt_frame frame )
	{
		mlt_audio_format format = mlt_audio_s16;
		int frequency = bmdAudioSampleRate48kHz;
		int samples = mlt_sample_calculator( m_fps, frequency, m_count );
		int16_t *pcm = 0;

		if ( !mlt_frame_get_audio( frame, (void**) &pcm, &format, &frequency, &m_channels, &samples ) )
		{
			int count = samples;

			if ( !m_isPrerolling )
			{
				uint32_t audioCount = 0;
				uint32_t videoCount = 0;

				// Check for resync
				m_deckLinkOutput->GetBufferedAudioSampleFrameCount( &audioCount );
				m_deckLinkOutput->GetBufferedVideoFrameCount( &videoCount );

				// Underflow typically occurs during non-normal speed playback.
				if ( audioCount < 1 || videoCount < 1 )
				{
					// Upon switching to normal playback, buffer some frames faster than realtime.
					mlt_log_info( getConsumer(), "buffer underrun: audio buf %u video buf %u frames\n", audioCount, videoCount );
					m_prerollCounter = 0;
				}

				// While rebuffering
				if ( isBuffering() )
				{
					// Only append audio to reach the ideal level and not overbuffer.
					int ideal = ( m_preroll - 1 ) * bmdAudioSampleRate48kHz / m_fps;
					int actual = m_fifo->used / m_channels + audioCount;
					int diff = ideal / 2 - actual;
					count = diff < 0 ? 0 : diff < count ? diff : count;
				}
			}
			if ( count > 0 )
				sample_fifo_append( m_fifo, pcm, count * m_channels );
		}
	}

	bool createFrame()
	{
		BMDPixelFormat format = m_isKeyer? bmdFormat8BitARGB : bmdFormat8BitYUV;
		IDeckLinkMutableVideoFrame* frame = 0;
		uint8_t *buffer = 0;
		int stride = m_width * ( m_isKeyer? 4 : 2 );

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
		mlt_log_debug( getConsumer(), "created video frame\n" );
		mlt_deque_push_back( m_videoFrameQ, frame );

		return true;
	}

	void renderVideo()
	{
		mlt_image_format format = m_isKeyer? mlt_image_rgb24a : mlt_image_yuv422;
		uint8_t* image = 0;

		if ( !mlt_frame_get_image( m_frame, &image, &format, &m_width, &m_height, 0 ) )
		{
			IDeckLinkMutableVideoFrame* decklinkFrame = (IDeckLinkMutableVideoFrame*) mlt_deque_pop_back( m_videoFrameQ );
			uint8_t* buffer = 0;
			int stride = m_width * ( m_isKeyer? 4 : 2 );

			decklinkFrame->GetBytes( (void**) &buffer );
			if ( buffer )
			{
				int progressive = mlt_properties_get_int( MLT_FRAME_PROPERTIES( m_frame ), "progressive" );

				if ( !m_isKeyer )
				{
					// Normal non-keyer playout - needs byte swapping
					if ( !progressive && m_displayMode->GetFieldDominance() == bmdUpperFieldFirst )
						// convert lower field first to top field first
						swab( image, buffer + stride, stride * ( m_height - 1 ) );
					else
						swab( image, buffer, stride * m_height );
				}
				else if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES( m_frame ), "test_image" ) )
				{
					// Normal keyer output
					int y = m_height + 1;
					uint32_t* s = (uint32_t*) image;
					uint32_t* d = (uint32_t*) buffer;

					if ( !progressive && m_displayMode->GetFieldDominance() == bmdUpperFieldFirst )
					{
						// Correct field order
						m_height--;
						d += stride;
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
				m_deckLinkOutput->ScheduleVideoFrame( decklinkFrame, m_count * m_duration, m_duration, m_timescale );
			}
			mlt_deque_push_front( m_videoFrameQ, decklinkFrame );
		}
	}

	HRESULT render( mlt_frame frame )
	{
		HRESULT result = S_OK;

		// Get the audio
		double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "_speed" );
		if ( m_isAudio && speed == 1.0 )
			renderAudio( frame );
		
		// Create video frames while pre-rolling
		if ( m_isPrerolling )
		{
			if ( !createFrame() )
			{
				mlt_log_error( getConsumer(), "failed to create video frame\n" );
				return S_FALSE;
			}
		}
		
		if ( mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "rendered") )
		{
			// Close the previous frame and use the new one
			mlt_frame_close( m_frame );
			m_frame = frame;
		}
		else
		{
			if ( !m_frame )
				m_frame = frame;
			// Reuse the last frame
			mlt_log_verbose( getConsumer(), "dropped video frame %u\n", ++m_dropped );
		}

		// Get the video
		renderVideo();
		++m_count;

		// Check for end of pre-roll
		if ( ++m_prerollCounter > m_preroll && m_isPrerolling )
		{
			// Start audio and video output
			if ( m_isAudio )
				m_deckLinkOutput->EndAudioPreroll();
			m_deckLinkOutput->StartScheduledPlayback( 0, m_timescale, 1.0 );
			m_isPrerolling = false;
		}

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
		wakeup();
		
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
	{
		return mlt_consumer_is_stopped( getConsumer() ) ? S_FALSE : S_OK;
	}
	
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples( bool preroll )
	{
		// Provide more audio samples to the DeckLink API
		HRESULT result = S_OK;

		uint32_t count = m_fifo->used / m_channels;
		uint32_t buffered = count;

		if ( count
			// Stay under preferred buffer level
			&& ( S_OK == m_deckLinkOutput->GetBufferedAudioSampleFrameCount( &buffered ) )
			&& buffered < m_maxAudioBuffer )
		{
			uint32_t written = 0;
			
			buffered = m_maxAudioBuffer - buffered;
			count = buffered > count ? count : buffered;
			result = m_deckLinkOutput->ScheduleAudioSamples( m_fifo->buffer, count, 0, 0, &written );
			if ( written )
				sample_fifo_remove( m_fifo, written * m_channels );
		}

		return result;
	}
};

/** The main thread.
 */

static void *run( void *arg )
{
	// Map the argument to the object
	DeckLinkConsumer* decklink = (DeckLinkConsumer*) arg;
	mlt_consumer consumer = decklink->getConsumer();
	
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Convenience functionality
	int terminate_on_pause = mlt_properties_get_int( properties, "terminate_on_pause" );
	int terminated = 0;

	// Frame and size
	mlt_frame frame = NULL;
	
	// Loop while running
	while ( !terminated && mlt_properties_get_int( properties, "running" ) )
	{
		// Get the frame
		if ( ( frame = mlt_consumer_rt_frame( consumer ) ) )
		{
			// Check for termination
			if ( terminate_on_pause )
				terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

			decklink->render( frame );
			if ( !decklink->isBuffering() )
				decklink->wait();
			mlt_events_fire( properties, "consumer-frame-show", frame, NULL );
		}
	}

	// Indicate that the consumer is stopped
	decklink->stop();
	mlt_properties_set_int( properties, "running", 0 );
	mlt_consumer_stopped( consumer );

	return NULL;
}

/** Start the consumer.
 */

static int start( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );
	DeckLinkConsumer* decklink = (DeckLinkConsumer*) consumer->child;
	int result = decklink->start( mlt_properties_get_int( properties, "preroll" ) ) ? 0 : 1;

	// Check that we're not already running
	if ( !result && !mlt_properties_get_int( properties, "running" ) )
	{
		// Allocate a thread
		pthread_t *pthread = (pthread_t*) calloc( 1, sizeof( pthread_t ) );

		// Assign the thread to properties
		mlt_properties_set_data( properties, "pthread", pthread, sizeof( pthread_t ), free, NULL );

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );
		mlt_properties_set_int( properties, "joined", 0 );

		// Create the thread
		pthread_create( pthread, NULL, run, consumer->child );
	}
	return result;
}

/** Stop the consumer.
 */

static int stop( mlt_consumer consumer )
{
	// Get the properties
	mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

	// Check that we're running
	if ( !mlt_properties_get_int( properties, "joined" ) )
	{
		// Get the thread
		pthread_t *pthread = (pthread_t*) mlt_properties_get_data( properties, "pthread", NULL );
		
		if ( pthread )
		{
			// Stop the thread
			mlt_properties_set_int( properties, "running", 0 );
			mlt_properties_set_int( properties, "joined", 1 );
	
			// Wait for termination
			pthread_join( *pthread, NULL );
		}
	}

	return 0;
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
