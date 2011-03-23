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
#include "DeckLinkAPI.h"

const unsigned PREROLL_MINIMUM = 3;

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
	IDeckLinkMutableVideoFrame* m_videoFrame;
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
		
	IDeckLinkDisplayMode* getDisplayMode()
	{
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( &m_consumer ) );
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
				mlt_log_verbose( &m_consumer, "BMD mode %dx%d %.3f fps prog %d\n", m_width, m_height, m_fps, p );
				
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
		if ( m_deckLinkOutput )
			m_deckLinkOutput->Release();
		if ( m_deckLink )
			m_deckLink->Release();
		if ( m_videoFrameQ )
			mlt_deque_close( m_videoFrameQ );
	}
	
	bool open( unsigned card = 0 )
	{
		IDeckLinkIterator* deckLinkIterator = CreateDeckLinkIteratorInstance();
		unsigned i = 0;
		
		if ( !deckLinkIterator )
		{
			mlt_log_verbose( NULL, "The DeckLink drivers not installed.\n" );
			return false;
		}
		
		// Connect to the Nth DeckLink instance
		do {
			if ( deckLinkIterator->Next( &m_deckLink ) != S_OK )
			{
				mlt_log_verbose( NULL, "DeckLink card not found\n" );
				deckLinkIterator->Release();
				return false;
			}
		} while ( ++i <= card );
		
		// Obtain the audio/video output interface (IDeckLinkOutput)
		if ( m_deckLink->QueryInterface( IID_IDeckLinkOutput, (void**)&m_deckLinkOutput ) != S_OK )
		{
			mlt_log_verbose( NULL, "No DeckLink cards support output\n" );
			m_deckLink->Release();
			m_deckLink = 0;
			deckLinkIterator->Release();
			return false;
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
		m_displayMode = getDisplayMode();
		if ( !m_displayMode )
		{
			mlt_log_error( &m_consumer, "Profile is not compatible with decklink.\n" );
			return false;
		}
		
		// Set the video output mode
		if ( S_OK != m_deckLinkOutput->EnableVideoOutput( m_displayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) )
		{
			mlt_log_error( &m_consumer, "Failed to enable video output\n" );
			return false;
		}
		
		// Set the audio output mode
		m_channels = 2;
		if ( S_OK != m_deckLinkOutput->EnableAudioOutput( bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger,
			m_channels, bmdAudioOutputStreamContinuous ) )
		{
			mlt_log_error( &m_consumer, "Failed to enable audio output\n" );
			stop();
			return false;
		}
		m_fifo = sample_fifo_init();
		
		// Preroll
		m_isPrerolling = true;
		m_prerollCounter = 0;
		m_preroll = preroll < PREROLL_MINIMUM ? PREROLL_MINIMUM : preroll;
		m_count = 0;
		m_deckLinkOutput->BeginAudioPreroll();
		m_frame = 0;
		m_dropped = 0;
		
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
			m_videoFrame = (IDeckLinkMutableVideoFrame*) mlt_deque_pop_back( m_videoFrameQ );
			m_videoFrame->Release();
		}
		m_videoFrame = 0;
		if ( m_fifo ) sample_fifo_close( m_fifo );
		mlt_frame_close( m_frame );
	}
	
	void createFrame()
	{
		m_videoFrame = 0;
		// Generate a DeckLink video frame
		if ( S_OK != m_deckLinkOutput->CreateVideoFrame( m_width, m_height,
			m_width * 2, bmdFormat8BitYUV, bmdFrameFlagDefault, &m_videoFrame ) )
		{
			mlt_log_verbose( &m_consumer, "Failed to create video frame\n" );
			stop();
			return;
		}
		
		// Make the first line black for field order correction.
		uint8_t *buffer = 0;
		if ( S_OK == m_videoFrame->GetBytes( (void**) &buffer ) && buffer )
		{
			for ( int i = 0; i < m_width; i++ )
			{
				*buffer++ = 128;
				*buffer++ = 16;
			}
		}
		mlt_log_debug( &m_consumer, "created video frame\n" );
		mlt_deque_push_back( m_videoFrameQ, m_videoFrame );
	}

	HRESULT render( mlt_frame frame )
	{
		HRESULT result = S_OK;
		// Get the audio		
		double speed = mlt_properties_get_double( MLT_FRAME_PROPERTIES(frame), "_speed" );
		if ( speed == 1.0 )
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
						mlt_log_info( &m_consumer, "buffer underrun: audio buf %u video buf %u frames\n", audioCount, videoCount );
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
		
		// Create video frames while pre-rolling
		if ( m_isPrerolling )
		{
			createFrame();
			if ( !m_videoFrame )
			{
				mlt_log_error( &m_consumer, "failed to create video frame\n" );
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
			// Reuse the last frame
			mlt_log_verbose( &m_consumer, "dropped video frame %u\n", ++m_dropped );
		}

		// Get the video
		mlt_image_format format = mlt_image_yuv422;
		uint8_t* image = 0;
		uint8_t* buffer = 0;
		if ( !mlt_frame_get_image( m_frame, &image, &format, &m_width, &m_height, 0 ) )
		{
			m_videoFrame = (IDeckLinkMutableVideoFrame*) mlt_deque_pop_back( m_videoFrameQ );
			m_videoFrame->GetBytes( (void**) &buffer );
			if ( m_displayMode->GetFieldDominance() == bmdUpperFieldFirst )
				// convert lower field first to top field first
				swab( image, buffer + m_width * 2, m_width * ( m_height - 1 ) * 2 );
			else
				swab( image, buffer, m_width * m_height * 2 );
			m_deckLinkOutput->ScheduleVideoFrame( m_videoFrame, m_count * m_duration, m_duration, m_timescale );
			mlt_deque_push_front( m_videoFrameQ, m_videoFrame );
		}
		++m_count;

		// Check for end of pre-roll
		if ( ++m_prerollCounter > m_preroll && m_isPrerolling )
		{
			// Start audio and video output
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
		return mlt_consumer_is_stopped( &m_consumer ) ? S_FALSE : S_OK;
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
		frame = mlt_consumer_rt_frame( consumer );

		// Check for termination
		if ( terminate_on_pause && frame )
			terminated = mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0;

		// Check that we have a frame to work with
		if ( frame )
		{
			decklink->render( frame );
			if ( !decklink->isBuffering() )
				decklink->wait();
			
			// Close the frame
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


MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "decklink", consumer_decklink_init );
}

} // extern C
