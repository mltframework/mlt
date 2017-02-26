/*
 * consumer_decklink.cpp -- output through Blackmagic Design DeckLink
 * Copyright (C) 2010-2017 Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define __STDC_FORMAT_MACROS  /* see inttypes.h */
#include <framework/mlt.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>
#include <pthread.h>
#include "common.h"

#define SWAB_SLICED_ALIGN_POW 5
static int swab_sliced( int id, int idx, int jobs, void* cookie )
{
	unsigned char** args = (unsigned char**)cookie;
	ssize_t sz = (ssize_t)args[2];
	ssize_t bsz = ( ( sz / jobs + ( 1 << SWAB_SLICED_ALIGN_POW ) - 1 ) >> SWAB_SLICED_ALIGN_POW ) << SWAB_SLICED_ALIGN_POW;
	ssize_t offset = bsz * idx;

	if ( offset < sz )
	{
		if ( ( offset + bsz ) > sz )
			bsz = sz - offset;

		swab2( args[0] + offset, args[1] + offset, bsz );
	}

	return 0;
};

static const unsigned PREROLL_MINIMUM = 3;

enum
{
	OP_NONE = 0,
	OP_OPEN,
	OP_START,
	OP_STOP,
	OP_EXIT
};

class DeckLinkConsumer
	: public IDeckLinkVideoOutputCallback
	, public IDeckLinkAudioOutputCallback
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
	bool                        m_isAudio;
	int                         m_isKeyer;
	IDeckLinkKeyer*             m_deckLinkKeyer;
	bool                        m_terminate_on_pause;
	uint32_t                    m_preroll;
	uint32_t                    m_acnt;
	uint32_t                    m_reprio;

	mlt_deque                   m_aqueue;
	pthread_mutex_t             m_aqueue_lock;
	mlt_deque                   m_frames;

	pthread_mutex_t             m_op_lock;
	pthread_mutex_t             m_op_arg_mutex;
	pthread_cond_t              m_op_arg_cond;
	int                         m_op_id;
	int                         m_op_res;
	int                         m_op_arg;
	pthread_t                   m_op_thread;
	bool                        m_sliced_swab;

	IDeckLinkDisplayMode* getDisplayMode()
	{
		mlt_profile profile = mlt_service_profile( MLT_CONSUMER_SERVICE( getConsumer() ) );
		IDeckLinkDisplayModeIterator* iter = NULL;
		IDeckLinkDisplayMode* mode = NULL;
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

				if ( m_width == profile->width && p == profile->progressive
					 && (int) m_fps == (int) mlt_profile_fps( profile )
					 && ( m_height == profile->height || ( m_height == 486 && profile->height == 480 ) ) )
					result = mode;
				else
					SAFE_RELEASE( mode );
			}
			SAFE_RELEASE( iter );
		}

		return result;
	}

public:
	mlt_consumer getConsumer()
		{ return &m_consumer; }

	DeckLinkConsumer()
	{
		pthread_mutexattr_t mta;

		m_displayMode = NULL;
		m_deckLinkKeyer = NULL;
		m_deckLinkOutput = NULL;
		m_deckLink = NULL;

		m_aqueue = mlt_deque_init();
		m_frames = mlt_deque_init();

		// operation locks
		m_op_id = OP_NONE;
		m_op_arg = 0;
		pthread_mutexattr_init( &mta );
		pthread_mutexattr_settype( &mta, PTHREAD_MUTEX_RECURSIVE );
		pthread_mutex_init( &m_op_lock, &mta );
		pthread_mutex_init( &m_op_arg_mutex, &mta );
		pthread_mutex_init( &m_aqueue_lock, &mta );
		pthread_mutexattr_destroy( &mta );
		pthread_cond_init( &m_op_arg_cond, NULL );
		pthread_create( &m_op_thread, NULL, op_main, this );
	}

	virtual ~DeckLinkConsumer()
	{
		mlt_log_debug( getConsumer(), "%s: entering\n",  __FUNCTION__ );

		SAFE_RELEASE( m_displayMode );
		SAFE_RELEASE( m_deckLinkKeyer );
		SAFE_RELEASE( m_deckLinkOutput );
		SAFE_RELEASE( m_deckLink );

		mlt_deque_close( m_aqueue );
		mlt_deque_close( m_frames );

		op(OP_EXIT, 0);
		mlt_log_debug( getConsumer(), "%s: waiting for op thread\n", __FUNCTION__ );
		pthread_join(m_op_thread, NULL);
		mlt_log_debug( getConsumer(), "%s: finished op thread\n", __FUNCTION__ );

		pthread_mutex_destroy( &m_aqueue_lock );
		pthread_mutex_destroy(&m_op_lock);
		pthread_mutex_destroy(&m_op_arg_mutex);
		pthread_cond_destroy(&m_op_arg_cond);

		mlt_log_debug( getConsumer(), "%s: exiting\n", __FUNCTION__ );
	}

	int op(int op_id, int arg)
	{
		int r;

		// lock operation mutex
		pthread_mutex_lock(&m_op_lock);

		mlt_log_debug( getConsumer(), "%s: op_id=%d\n", __FUNCTION__, op_id );

		// notify op id
		pthread_mutex_lock(&m_op_arg_mutex);
		m_op_id = op_id;
		m_op_arg = arg;
		pthread_cond_signal(&m_op_arg_cond);
		pthread_mutex_unlock(&m_op_arg_mutex);

		// wait op done
		pthread_mutex_lock(&m_op_arg_mutex);
		while(OP_NONE != m_op_id)
			pthread_cond_wait(&m_op_arg_cond, &m_op_arg_mutex);
		pthread_mutex_unlock(&m_op_arg_mutex);

		// save result
		r = m_op_res;

		mlt_log_debug( getConsumer(), "%s: r=%d\n", __FUNCTION__, r );

		// unlock operation mutex
		pthread_mutex_unlock(&m_op_lock);

		return r;
	}

protected:

	static void* op_main(void* thisptr)
	{
		DeckLinkConsumer* d = static_cast<DeckLinkConsumer*>(thisptr);

		mlt_log_debug( d->getConsumer(), "%s: entering\n", __FUNCTION__ );

		for (;;)
		{
			int o, r = 0;

			// wait op command
			pthread_mutex_lock ( &d->m_op_arg_mutex );
			while ( OP_NONE == d->m_op_id )
				pthread_cond_wait( &d->m_op_arg_cond, &d->m_op_arg_mutex );
			pthread_mutex_unlock( &d->m_op_arg_mutex );
			o = d->m_op_id;

			mlt_log_debug( d->getConsumer(), "%s:%d d->m_op_id=%d\n", __FUNCTION__, __LINE__, d->m_op_id );

			switch ( d->m_op_id )
			{
				case OP_OPEN:
					r = d->m_op_res = d->open( d->m_op_arg );
					break;

				case OP_START:
					r = d->m_op_res = d->start( d->m_op_arg );
					break;

				case OP_STOP:
					r = d->m_op_res = d->stop();
					break;
			};

			// notify op done
			pthread_mutex_lock( &d->m_op_arg_mutex );
			d->m_op_id = OP_NONE;
			pthread_cond_signal( &d->m_op_arg_cond );
			pthread_mutex_unlock( &d->m_op_arg_mutex );

			// post for async
			if ( OP_START == o && r )
				d->preroll();

			if ( OP_EXIT == o )
			{
				mlt_log_debug( d->getConsumer(), "%s: exiting\n", __FUNCTION__ );
				return NULL;
			}
		};

		return NULL;
	}

	bool open( unsigned card = 0 )
	{
		unsigned i = 0;
#ifdef _WIN32
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
			mlt_log_warning( getConsumer(), "The DeckLink drivers not installed.\n" );
			return false;
		}
#else
		IDeckLinkIterator* deckLinkIterator = CreateDeckLinkIteratorInstance();

		if ( !deckLinkIterator )
		{
			mlt_log_warning( getConsumer(), "The DeckLink drivers not installed.\n" );
			return false;
		}
#endif

		// Connect to the Nth DeckLink instance
		for ( i = 0; deckLinkIterator->Next( &m_deckLink ) == S_OK ; i++)
		{
			if( i == card )
				break;
			else
				SAFE_RELEASE( m_deckLink );
		}
		SAFE_RELEASE( deckLinkIterator );
		if ( !m_deckLink )
		{
			mlt_log_error( getConsumer(), "DeckLink card not found\n" );
			return false;
		}

		// Obtain the audio/video output interface (IDeckLinkOutput)
		if ( m_deckLink->QueryInterface( IID_IDeckLinkOutput, (void**)&m_deckLinkOutput ) != S_OK )
		{
			mlt_log_error( getConsumer(), "No DeckLink cards support output\n" );
			SAFE_RELEASE( m_deckLink );
			return false;
		}

		// Get the keyer interface
		IDeckLinkAttributes *deckLinkAttributes = 0;
		if ( m_deckLink->QueryInterface( IID_IDeckLinkAttributes, (void**) &deckLinkAttributes ) == S_OK )
		{
#ifdef _WIN32
			BOOL flag = FALSE;
#else
			bool flag = false;
#endif
			if ( deckLinkAttributes->GetFlag( BMDDeckLinkSupportsInternalKeying, &flag ) == S_OK && flag )
			{
				if ( m_deckLink->QueryInterface( IID_IDeckLinkKeyer, (void**) &m_deckLinkKeyer ) != S_OK )
				{
					mlt_log_error( getConsumer(), "Failed to get keyer\n" );
					SAFE_RELEASE( m_deckLinkOutput );
					SAFE_RELEASE( m_deckLink );
					return false;
				}
			}
			SAFE_RELEASE( deckLinkAttributes );
		}

		// Provide this class as a delegate to the audio and video output interfaces
		m_deckLinkOutput->SetScheduledFrameCompletionCallback( this );
		m_deckLinkOutput->SetAudioCallback( this );

		return true;
	}

	int preroll()
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		mlt_log_debug( getConsumer(), "%s: starting\n", __FUNCTION__ );

		if ( !mlt_properties_get_int( properties, "running" ) )
			return 0;

		mlt_log_verbose( getConsumer(), "preroll %u frames\n", m_preroll );

		// preroll frames
		for ( unsigned i = 0; i < m_preroll ; i++ )
			ScheduleNextFrame( true );

		// start audio preroll
		if ( m_isAudio )
			m_deckLinkOutput->BeginAudioPreroll( );
		else
			m_deckLinkOutput->StartScheduledPlayback( 0, m_timescale, 1.0 );

		mlt_log_debug( getConsumer(), "%s: exiting\n", __FUNCTION__ );

		return 0;
	}

	bool start( unsigned preroll )
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		// Initialize members
		m_count = 0;
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
		mlt_properties_set_int( properties, "top_field_first", m_displayMode->GetFieldDominance() == bmdUpperFieldFirst );

		// Set the keyer
		if ( m_deckLinkKeyer && ( m_isKeyer = mlt_properties_get_int( properties, "keyer" ) ) )
		{
			bool external = ( m_isKeyer == 2 );
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
		if ( S_OK != m_deckLinkOutput->EnableVideoOutput( m_displayMode->GetDisplayMode(),
			(BMDVideoOutputFlags) (bmdVideoOutputFlagDefault | bmdVideoOutputRP188 | bmdVideoOutputVITC) ) )
		{
			mlt_log_error( getConsumer(), "Failed to enable video output\n" );
			return false;
		}

		// Set the audio output mode
		if ( m_isAudio && S_OK != m_deckLinkOutput->EnableAudioOutput( bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger,
			m_channels, bmdAudioOutputStreamTimestamped ) )
		{
			mlt_log_error( getConsumer(), "Failed to enable audio output\n" );
			stop();
			return false;
		}

		m_preroll = preroll;
		m_reprio = 2;

		for ( unsigned i = 0; i < ( m_preroll + 2 ) ; i++)
		{
			IDeckLinkMutableVideoFrame* frame;

			// Generate a DeckLink video frame
			if ( S_OK != m_deckLinkOutput->CreateVideoFrame( m_width, m_height,
				m_width * ( m_isKeyer? 4 : 2 ), m_isKeyer? bmdFormat8BitARGB : bmdFormat8BitYUV, bmdFrameFlagDefault, &frame ) )
			{
				mlt_log_error( getConsumer(), "%s: CreateVideoFrame (%d) failed\n", __FUNCTION__, i );
				return false;
			}

			mlt_deque_push_back( m_frames, reinterpret_cast<IDeckLinkMutableVideoFrame*>(frame) );
		}

		// Set the running state
		mlt_properties_set_int( properties, "running", 1 );

		return true;
	}

	bool stop()
	{
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		mlt_log_debug( getConsumer(), "%s: starting\n", __FUNCTION__ );

		// Stop the audio and video output streams immediately
		if ( m_deckLinkOutput )
		{
			m_deckLinkOutput->StopScheduledPlayback( 0, 0, 0 );
			m_deckLinkOutput->DisableAudioOutput();
			m_deckLinkOutput->DisableVideoOutput();
		}

		pthread_mutex_lock( &m_aqueue_lock );
		while ( mlt_frame frame = (mlt_frame) mlt_deque_pop_back( m_aqueue ) )
			mlt_frame_close( frame );
		pthread_mutex_unlock( &m_aqueue_lock );

		while ( IDeckLinkMutableVideoFrame* frame = (IDeckLinkMutableVideoFrame*) mlt_deque_pop_back( m_frames ) )
			SAFE_RELEASE( frame );

		// set running state is 0
		mlt_properties_set_int( properties, "running", 0 );

		mlt_consumer_stopped( getConsumer() );

		mlt_log_debug( getConsumer(), "%s: exiting\n", __FUNCTION__ );

		return true;
	}

	void renderAudio( mlt_frame frame )
	{
		mlt_properties properties;
		properties = MLT_FRAME_PROPERTIES( frame );
		mlt_properties_set_int64( properties, "m_count", m_count);
		mlt_properties_inc_ref( properties );
		pthread_mutex_lock( &m_aqueue_lock );
		mlt_deque_push_back( m_aqueue, frame );
		mlt_log_debug( getConsumer(), "%s:%d frame=%p, len=%d\n", __FUNCTION__, __LINE__, frame, mlt_deque_count( m_aqueue ));
		pthread_mutex_unlock( &m_aqueue_lock );
	}

	bool createFrame( IDeckLinkMutableVideoFrame** decklinkFrame )
	{
		IDeckLinkMutableVideoFrame* frame = (IDeckLinkMutableVideoFrame*)mlt_deque_pop_front( m_frames );;

		*decklinkFrame = frame;

		return ( !frame ) ? false : true;
	}

	void renderVideo( mlt_frame frame )
	{
		HRESULT hr;
		mlt_image_format format = m_isKeyer? mlt_image_rgb24a : mlt_image_yuv422;
		uint8_t* image = 0;
		int rendered = mlt_properties_get_int( MLT_FRAME_PROPERTIES(frame), "rendered");
		mlt_properties consumer_properties = MLT_CONSUMER_PROPERTIES( getConsumer() );
		int height = m_height;
		IDeckLinkMutableVideoFrame* m_decklinkFrame = NULL;

		mlt_log_debug( getConsumer(), "%s: entering\n", __FUNCTION__ );

		m_sliced_swab = mlt_properties_get_int( consumer_properties, "sliced_swab" );

		if ( rendered && !mlt_frame_get_image( frame, &image, &format, &m_width, &height, 0 ) )
		{
			uint8_t* buffer = 0;
			int stride = m_width * ( m_isKeyer? 4 : 2 );

			if ( createFrame( &m_decklinkFrame ) )
				m_decklinkFrame->GetBytes( (void**) &buffer );

			if ( buffer )
			{
				// NTSC SDI is always 486 lines
				if ( m_height == 486 && height == 480 )
				{
					// blank first 6 lines
					if ( m_isKeyer )
					{
						memset( buffer, 0, stride * 6 );
						buffer += stride * 6;
					}
					else for ( int i = 0; i < m_width * 6; i++ )
					{
						*buffer++ = 128;
						*buffer++ = 16;
					}
				}
				if ( !m_isKeyer )
				{
					unsigned char *arg[3] = { image, buffer };
					ssize_t size = stride * height;

					// Normal non-keyer playout - needs byte swapping
					if ( !m_sliced_swab )
						swab2( arg[0], arg[1], size );
					else
					{
						arg[2] = (unsigned char*)size;
						mlt_slices_run_fifo( 0, swab_sliced, arg);
					}
				}
				else if ( !mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "test_image" ) )
				{
					// Normal keyer output
					int y = height + 1;
					uint32_t* s = (uint32_t*) image;
					uint32_t* d = (uint32_t*) buffer;

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
					memset( buffer, 0, stride * height );
				}
			}
		}
		if ( m_decklinkFrame )
		{
			char* vitc;

			// set timecode
			vitc = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "meta.attr.vitc.markup" );
			if( vitc )
			{
				int h, m, s, f;
				if ( 4 == sscanf( vitc, "%d:%d:%d:%d", &h, &m, &s, &f ) )
					m_decklinkFrame->SetTimecodeFromComponents(bmdTimecodeVITC,
						h, m, s, f, bmdTimecodeFlagDefault);
			}

			// set userbits
			vitc = mlt_properties_get( MLT_FRAME_PROPERTIES( frame ), "meta.attr.vitc.userbits" );
			if( vitc )
				m_decklinkFrame->SetTimecodeUserBits(bmdTimecodeVITC,
					mlt_properties_get_int( MLT_FRAME_PROPERTIES( frame ), "meta.attr.vitc.userbits" ));

			hr = m_deckLinkOutput->ScheduleVideoFrame( m_decklinkFrame, m_count * m_duration, m_duration, m_timescale );
			if ( S_OK != hr )
				mlt_log_error( getConsumer(), "%s:%d: ScheduleVideoFrame failed, hr=%.8X \n", __FUNCTION__, __LINE__, unsigned(hr) );
			else
				mlt_log_debug( getConsumer(), "%s: ScheduleVideoFrame SUCCESS\n", __FUNCTION__ );
		}
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

	void reprio( int target )
	{
		int r;
		pthread_t thread;
		pthread_attr_t tattr;
		struct sched_param param;
		mlt_properties properties;

		if( m_reprio & target )
			return;

		m_reprio |= target;

		properties = MLT_CONSUMER_PROPERTIES( getConsumer() );

		if ( !mlt_properties_get( properties, "priority" ) )
			return;

		pthread_attr_init(&tattr);
		pthread_attr_setschedpolicy(&tattr, SCHED_FIFO);

		if ( !strcmp( "max", mlt_properties_get( properties, "priority" ) ) )
			param.sched_priority = sched_get_priority_max(SCHED_FIFO) - 1;
		else if ( !strcmp( "min", mlt_properties_get( properties, "priority" ) ) )
			param.sched_priority = sched_get_priority_min(SCHED_FIFO) + 1;
		else
			param.sched_priority = mlt_properties_get_int( properties, "priority" );

		pthread_attr_setschedparam(&tattr, &param);

		thread = pthread_self();

		r = pthread_setschedparam(thread, SCHED_FIFO, &param);
		if( r )
			mlt_log_error( getConsumer(),
				"%s: [%d] pthread_setschedparam returned %d\n", __FUNCTION__, target, r);
		else
			mlt_log_verbose( getConsumer(),
				"%s: [%d] param.sched_priority=%d\n", __FUNCTION__, target, param.sched_priority);
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

#ifdef _WIN32
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples ( BOOL preroll )
#else
	virtual HRESULT STDMETHODCALLTYPE RenderAudioSamples ( bool preroll )
#endif
	{
		pthread_mutex_lock( &m_aqueue_lock );
		mlt_log_debug( getConsumer(), "%s: ENTERING preroll=%d, len=%d\n", __FUNCTION__, (int)preroll, mlt_deque_count( m_aqueue ));
		mlt_frame frame = (mlt_frame) mlt_deque_pop_front( m_aqueue );
		pthread_mutex_unlock( &m_aqueue_lock );

		reprio( 2 );

		if ( frame )
		{
			mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
			uint64_t m_count = mlt_properties_get_int64( properties, "m_count" );
			mlt_audio_format format = mlt_audio_s16;
			int frequency = bmdAudioSampleRate48kHz;
			int samples = mlt_sample_calculator( m_fps, frequency, m_count );
			int16_t *pcm = 0;

			if ( !mlt_frame_get_audio( frame, (void**) &pcm, &format, &frequency, &m_channels, &samples ) )
			{
				HRESULT hr;
				mlt_log_debug( getConsumer(), "%s:%d, samples=%d, channels=%d, freq=%d\n",
					__FUNCTION__, __LINE__, samples, m_channels, frequency );
#ifdef _WIN32
#define DECKLINK_UNSIGNED_FORMAT "%lu"
				unsigned long written = 0;
#else
#define DECKLINK_UNSIGNED_FORMAT "%u"
				uint32_t written = 0;
#endif
				BMDTimeValue streamTime = m_count * frequency * m_duration / m_timescale;
#ifdef _WIN32
				hr = m_deckLinkOutput->ScheduleAudioSamples( pcm, samples, streamTime, frequency, (unsigned long*) &written );
#else
				hr = m_deckLinkOutput->ScheduleAudioSamples( pcm, samples, streamTime, frequency, &written );
#endif
				if ( S_OK != hr )
					mlt_log_error( getConsumer(), "%s:%d ScheduleAudioSamples failed, hr=%.8X \n", __FUNCTION__, __LINE__, unsigned(hr) );
				else
					mlt_log_debug( getConsumer(), "%s:%d ScheduleAudioSamples success " DECKLINK_UNSIGNED_FORMAT " samples\n", __FUNCTION__, __LINE__, written );
				if ( written != (uint32_t) samples )
					mlt_log_verbose( getConsumer(), "renderAudio: samples=%d, written=" DECKLINK_UNSIGNED_FORMAT "\n", samples, written );
			}
			else
				mlt_log_error( getConsumer(), "%s:%d mlt_frame_get_audio failed\n", __FUNCTION__, __LINE__);

			mlt_frame_close( frame );

			if ( !preroll )
				RenderAudioSamples ( preroll );
		}

		if ( preroll )
			m_deckLinkOutput->StartScheduledPlayback( 0, m_timescale, 1.0 );

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledFrameCompleted( IDeckLinkVideoFrame* completedFrame, BMDOutputFrameCompletionResult completed )
	{
		mlt_log_debug( getConsumer(), "%s: ENTERING\n", __FUNCTION__ );

		mlt_deque_push_back( m_frames, reinterpret_cast/*dynamic_cast*/<IDeckLinkMutableVideoFrame*>(completedFrame) );

		//  change priority of video callback thread
		reprio( 1 );

		// When a video frame has been released by the API, schedule another video frame to be output

		// ignore handler if frame was flushed
		if ( bmdOutputFrameFlushed == completed )
			return S_OK;

		// schedule next frame
		ScheduleNextFrame( false );

		// step forward frames counter if underrun
		if ( bmdOutputFrameDisplayedLate == completed )
		{
			mlt_log_verbose( getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDisplayedLate == completed\n" );
		}
		if ( bmdOutputFrameDropped == completed )
		{
			mlt_log_verbose( getConsumer(), "ScheduledFrameCompleted: bmdOutputFrameDropped == completed\n" );
			m_count++;
			ScheduleNextFrame( false );
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE ScheduledPlaybackHasStopped()
	{
		return mlt_consumer_is_stopped( getConsumer() ) ? S_FALSE : S_OK;
	}

	void ScheduleNextFrame( bool preroll )
	{
		// get the consumer
		mlt_consumer consumer = getConsumer();

		// Get the properties
		mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

		// Frame and size
		mlt_frame frame = NULL;

		mlt_log_debug( getConsumer(), "%s:%d: preroll=%d\n", __FUNCTION__, __LINE__, preroll);

		if( mlt_properties_get_int( properties, "running" ) || preroll )
		{
			mlt_log_timings_begin();
			frame = mlt_consumer_rt_frame( consumer );
			mlt_log_timings_end( NULL, "mlt_consumer_rt_frame" );
			if ( frame )
			{
				mlt_log_timings_begin();
				render( frame );
				mlt_log_timings_end( NULL, "render" );

				mlt_events_fire( properties, "consumer-frame-show", frame, NULL );

				// terminate on pause
				if ( m_terminate_on_pause &&
					mlt_properties_get_double( MLT_FRAME_PROPERTIES( frame ), "_speed" ) == 0.0 )
					stop();

				mlt_frame_close( frame );
			}
			else
				mlt_log_error( getConsumer(), "%s: mlt_consumer_rt_frame return NULL\n", __FUNCTION__ );
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
	return decklink->op( OP_START, mlt_properties_get_int( properties, "preroll" ) ) ? 0 : 1;
}

/** Stop the consumer.
 */

static int stop( mlt_consumer consumer )
{
	int r;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	// Get the properties
	DeckLinkConsumer* decklink = (DeckLinkConsumer*) consumer->child;
	r = decklink->op(OP_STOP, 0);

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );

	return r;
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
	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: entering\n", __FUNCTION__ );

	// Stop the consumer
	mlt_consumer_stop( consumer );

	// Close the parent
	consumer->close = NULL;
	mlt_consumer_close( consumer );

	// Free the memory
	delete (DeckLinkConsumer*) consumer->child;

	mlt_log_debug( MLT_CONSUMER_SERVICE(consumer), "%s: exiting\n", __FUNCTION__ );
}

extern "C" {

// Listen for the list_devices property to be set
static void on_property_changed( void*, mlt_properties properties, const char *name )
{
	IDeckLinkIterator* decklinkIterator = NULL;
	IDeckLink* decklink = NULL;
	IDeckLinkInput* decklinkOutput = NULL;
	int i = 0;

	if ( name && !strcmp( name, "list_devices" ) )
		mlt_event_block( (mlt_event) mlt_properties_get_data( properties, "list-devices-event", NULL ) );
	else
		return;

#ifdef _WIN32
	if ( FAILED( CoInitialize( NULL ) ) )
		return;
	if ( FAILED( CoCreateInstance( CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**) &decklinkIterator ) ) )
		return;
#else
	if ( !( decklinkIterator = CreateDeckLinkIteratorInstance() ) )
		return;
#endif
	for ( ; decklinkIterator->Next( &decklink ) == S_OK; i++ )
	{
		if ( decklink->QueryInterface( IID_IDeckLinkOutput, (void**) &decklinkOutput ) == S_OK )
		{
			DLString name = NULL;
			if ( decklink->GetModelName( &name ) == S_OK )
			{
				char *name_cstr = getCString( name );
				const char *format = "device.%d";
				char *key = (char*) calloc( 1, strlen( format ) + 1 );

				sprintf( key, format, i );
				mlt_properties_set( properties, key, name_cstr );
				free( key );
				freeDLString( name );
				freeCString( name_cstr );
			}
			SAFE_RELEASE( decklinkOutput );
		}
		SAFE_RELEASE( decklink );
	}
	SAFE_RELEASE( decklinkIterator );
	mlt_properties_set_int( properties, "devices", i );
}

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
		if ( decklink->op( OP_OPEN, arg? atoi(arg) : 0 ) )
		{
			consumer = decklink->getConsumer();
			mlt_properties properties = MLT_CONSUMER_PROPERTIES( consumer );

			// Setup callbacks
			consumer->close = close;
			consumer->start = start;
			consumer->stop = stop;
			consumer->is_stopped = is_stopped;
			mlt_properties_set( properties, "deinterlace_method", "onefield" );

			mlt_event event = mlt_events_listen( properties, properties, "property-changed", (mlt_listener) on_property_changed );
			mlt_properties_set_data( properties, "list-devices-event", event, 0, NULL, NULL );
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
