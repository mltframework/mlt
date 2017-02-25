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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <framework/mlt.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/time.h>
#include "common.h"

#include <framework/mlt_slices.h>

struct copy_lines_sliced_desc
{
	BMDPixelFormat in_fmt;
	mlt_image_format out_fmt;
	unsigned char *in_buffer, **out_buffers;
	int in_stride, *out_strides, w, h;
};

#define READ_PIXELS( a, b, c ) \
val  = *v210++;  \
*a++ = ( val & 0x3FF ) << 6; \
*b++ = ( ( val >> 10 ) & 0x3FF ) << 6;  \
*c++ = ( ( val >> 20 ) & 0x3FF ) << 6;

static int copy_lines_sliced_proc( int id, int idx, int jobs, void* cookie )
{
	int c, H, Y, i;
	struct copy_lines_sliced_desc *ctx = (struct copy_lines_sliced_desc*)cookie;

	H = ( ctx->h + jobs ) / jobs;
	Y = idx * H;
	H = MIN( H, ctx->h - Y );

	if ( ctx->in_fmt == bmdFormat10BitYUV ) // bmdFormat10BitYUV -> mlt_image_yuv422p16
	{
		for( i = 0; i < H; i++)
		{
			uint32_t val,
				*v210 = (uint32_t*)( ctx->in_buffer + ( Y + i ) * ctx->in_stride );
			uint16_t
				*y = (uint16_t*)( ctx->out_buffers[0] + ( Y + i ) * ctx->out_strides[0] ),
				*u = (uint16_t*)( ctx->out_buffers[1] + ( Y + i ) * ctx->out_strides[1] ),
				*v = (uint16_t*)( ctx->out_buffers[2] + ( Y + i ) * ctx->out_strides[2] );

			for( c = 0; c < ctx->w / 6; c++ )
			{
				READ_PIXELS( u, y, v );
				READ_PIXELS( y, u, y );
				READ_PIXELS( v, y, u );
				READ_PIXELS( y, v, y );
			}
		}
	}
	else // bmdFormat8BitYUV -> mlt_image_yuv422
	{
		if ( ctx->out_strides[0] == ctx->in_stride )
			swab2(ctx->in_buffer + Y * ctx->in_stride,
				ctx->out_buffers[0] + Y * ctx->out_strides[0],
				H * ctx->in_stride );
		else
			for(i = 0; i < H; i++ )
				swab2(ctx->in_buffer + ( Y + i ) * ctx->in_stride,
					ctx->out_buffers[0] + ( Y + i ) * ctx->out_strides[0],
						MIN( ctx->in_stride , ctx->out_strides[0] ) );
	}

	return 0;
}

static void copy_lines( BMDPixelFormat in_fmt, unsigned char* in_buffer, int in_stride,
	mlt_image_format out_fmt, unsigned char* out_buffers[4], int out_strides[4], int w, int h )
{
	struct copy_lines_sliced_desc ctx = { .in_fmt = in_fmt, .out_fmt = out_fmt, .in_buffer = in_buffer,
		.out_buffers = out_buffers, .in_stride = in_stride, .out_strides = out_strides };

	ctx.w = w; ctx.h = h;

	if ( h == 1 )
		copy_lines_sliced_proc( 0, 0, 1, &ctx );
	else
		mlt_slices_run_normal( mlt_slices_count_normal(), copy_lines_sliced_proc, &ctx );
}

static void fill_line( mlt_image_format out_fmt, unsigned char *in[4], int strides[4], int pattern )
{
	// TODO
}

class DeckLinkProducer
	: public IDeckLinkInputCallback
{
private:
	mlt_producer     m_producer;
	IDeckLink*       m_decklink;
	IDeckLinkInput*  m_decklinkInput;
	mlt_deque        m_queue;
	pthread_mutex_t  m_mutex;
	pthread_cond_t   m_condition;
	bool             m_started;
	int              m_dropped;
	bool             m_isBuffering;
	int              m_topFieldFirst;
	BMDPixelFormat   m_pixel_format;
	int              m_colorspace;
	int              m_vancLines;
	mlt_cache        m_cache;
	bool             m_reprio;

	BMDDisplayMode getDisplayMode( mlt_profile profile, int vancLines )
	{
		IDeckLinkDisplayModeIterator* iter = NULL;
		IDeckLinkDisplayMode* mode = NULL;
		BMDDisplayMode result = (BMDDisplayMode) bmdDisplayModeNotSupported;

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
				m_topFieldFirst = mode->GetFieldDominance() == bmdUpperFieldFirst;
				m_colorspace = ( mode->GetFlags() & bmdDisplayModeColorspaceRec709 ) ? 709 : 601;
				mlt_log_verbose( getProducer(), "BMD mode %dx%d %.3f fps prog %d tff %d\n", width, height, fps, p, m_topFieldFirst );

				if ( width == profile->width && p == profile->progressive
					 && ( height + vancLines == profile->height || ( height == 486 && profile->height == 480 + vancLines ) )
					 && (int) fps == (int) mlt_profile_fps( profile ) )
					result = mode->GetDisplayMode();
				SAFE_RELEASE( mode );
			}
			SAFE_RELEASE( iter );
		}

		return result;
	}

public:
	mlt_profile      m_new_input;

	void setProducer( mlt_producer producer )
		{ m_producer = producer; }

	mlt_producer getProducer() const
		{ return m_producer; }

	DeckLinkProducer()
	{
		m_producer = NULL;
		m_decklink = NULL;
		m_decklinkInput = NULL;
		m_new_input = NULL;
	}

	virtual ~DeckLinkProducer()
	{
		if ( m_queue )
		{
			stop();
			mlt_deque_close( m_queue );
			pthread_mutex_destroy( &m_mutex );
			pthread_cond_destroy( &m_condition );
			mlt_cache_close( m_cache );
		}
		SAFE_RELEASE( m_decklinkInput );
		SAFE_RELEASE( m_decklink );
	}

	bool open( unsigned card =  0 )
	{
		IDeckLinkIterator* decklinkIterator = NULL;
		try
		{
#ifdef _WIN32
			HRESULT result =  CoInitialize( NULL );
			if ( FAILED( result ) )
				throw "COM initialization failed";
			result = CoCreateInstance( CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**) &decklinkIterator );
			if ( FAILED( result ) )
				throw "The DeckLink drivers are not installed.";
#else
			decklinkIterator = CreateDeckLinkIteratorInstance();
			if ( !decklinkIterator )
				throw "The DeckLink drivers are not installed.";
#endif
			// Connect to the Nth DeckLink instance
			for ( unsigned i = 0; decklinkIterator->Next( &m_decklink ) == S_OK ; i++)
			{
				if ( i == card )
					break;
				else
					SAFE_RELEASE( m_decklink );
			}
			SAFE_RELEASE( decklinkIterator );
			if ( !m_decklink )
				throw "DeckLink card not found.";

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
			m_dropped = 0;
			m_isBuffering = true;
			m_cache = mlt_cache_init();

			// 3 covers YADIF and increasing framerate use cases
			mlt_cache_set_size( m_cache, 3 );
		}
		catch ( const char *error )
		{
			SAFE_RELEASE( m_decklinkInput );
			SAFE_RELEASE( m_decklink );
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
			// Initialize some members
			m_vancLines = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "vanc" );
			if ( m_vancLines == -1 )
				m_vancLines = profile->height <= 512 ? 26 : 32;

			if ( !profile )
				profile = mlt_service_profile( MLT_PRODUCER_SERVICE( getProducer() ) );

			// Get the display mode
			BMDDisplayMode displayMode = getDisplayMode( profile, m_vancLines );
			if ( displayMode == (BMDDisplayMode) bmdDisplayModeNotSupported )
			{
				mlt_log_info( getProducer(), "profile = %dx%d %f fps %s\n", profile->width, profile->height,
							  mlt_profile_fps( profile ), profile->progressive? "progressive" : "interlace" );
				throw "Profile is not compatible with decklink.";
			}

			// Determine if supports input format detection
#ifdef _WIN32
			BOOL doesDetectFormat = FALSE;
#else
			bool doesDetectFormat = false;
#endif
			IDeckLinkAttributes *decklinkAttributes = 0;
			if ( m_decklink->QueryInterface( IID_IDeckLinkAttributes, (void**) &decklinkAttributes ) == S_OK )
			{
				if ( decklinkAttributes->GetFlag( BMDDeckLinkSupportsInputFormatDetection, &doesDetectFormat ) != S_OK )
					doesDetectFormat = false;
				SAFE_RELEASE( decklinkAttributes );
			}
			mlt_log_verbose( getProducer(), "%s format detection\n", doesDetectFormat ? "supports" : "does not support" );

			// Enable video capture
			m_pixel_format = ( 10 == mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "bitdepth" ) )
				? bmdFormat10BitYUV : bmdFormat8BitYUV;
			BMDVideoInputFlags flags = doesDetectFormat ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;
			if ( S_OK != m_decklinkInput->EnableVideoInput( displayMode, m_pixel_format, flags ) )
				throw "Failed to enable video capture.";

			// Enable audio capture
			BMDAudioSampleRate sampleRate = bmdAudioSampleRate48kHz;
			BMDAudioSampleType sampleType = bmdAudioSampleType16bitInteger;
			int channels = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "channels" );
			if ( S_OK != m_decklinkInput->EnableAudioInput( sampleRate, sampleType, channels ) )
				throw "Failed to enable audio capture.";

			// Start capture
			m_dropped = 0;
			mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "dropped", m_dropped );
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
		if ( !m_started )
			return;
		m_started = false;

		// Release the wait in getFrame
		pthread_mutex_lock( &m_mutex );
		pthread_cond_broadcast( &m_condition );
		pthread_mutex_unlock( &m_mutex );

		m_decklinkInput->StopStreams();
		m_decklinkInput->DisableVideoInput();
		m_decklinkInput->DisableAudioInput();

		// Cleanup queue
		pthread_mutex_lock( &m_mutex );
		while ( mlt_frame frame = (mlt_frame) mlt_deque_pop_back( m_queue ) )
			mlt_frame_close( frame );
		pthread_mutex_unlock( &m_mutex );
	}

	mlt_frame getFrame()
	{
		struct timeval now;
		struct timespec tm;
		double fps = mlt_producer_get_fps( getProducer() );
		mlt_position position = mlt_producer_position( getProducer() );
		mlt_frame frame = mlt_cache_get_frame( m_cache, position );

		// Allow the buffer to fill to the requested initial buffer level.
		if ( m_isBuffering )
		{
			int prefill = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "prefill" );
			int buffer = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "buffer" );

			m_isBuffering = false;
			prefill = prefill > buffer ? buffer : prefill;
			pthread_mutex_lock( &m_mutex );
			while ( mlt_deque_count( m_queue ) < prefill )
			{
				// Wait up to buffer/fps seconds
				gettimeofday( &now, NULL );
				long usec = now.tv_sec * 1000000 + now.tv_usec;
				usec += 1000000 * buffer / fps;
				tm.tv_sec = usec / 1000000;
				tm.tv_nsec = (usec % 1000000) * 1000;
				if ( pthread_cond_timedwait( &m_condition, &m_mutex, &tm ) )
					break;
			}
			pthread_mutex_unlock( &m_mutex );
		}

		if ( !frame )
		{
			// Wait if queue is empty
			pthread_mutex_lock( &m_mutex );
			while ( mlt_deque_count( m_queue ) < 1 )
			{
				// Wait up to twice frame duration
				gettimeofday( &now, NULL );
				long usec = now.tv_sec * 1000000 + now.tv_usec;
				usec += 2000000 / fps;
				tm.tv_sec = usec / 1000000;
				tm.tv_nsec = (usec % 1000000) * 1000;
				if ( pthread_cond_timedwait( &m_condition, &m_mutex, &tm ) )
					// Stop waiting if error (timed out)
					break;
			}
			frame = ( mlt_frame ) mlt_deque_pop_front( m_queue );
			pthread_mutex_unlock( &m_mutex );

			// add to cache
			if ( frame )
			{
				mlt_frame_set_position( frame, position );
				mlt_cache_put_frame( m_cache, frame );
			}
		}

		// Set frame timestamp and properties
		if ( frame )
		{
			mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( getProducer() ) );
			mlt_properties properties = MLT_FRAME_PROPERTIES( frame );
			mlt_properties_set_int( properties, "progressive", profile->progressive );
			mlt_properties_set_int( properties, "meta.media.progressive", profile->progressive );
			mlt_properties_set_int( properties, "top_field_first", m_topFieldFirst );
			mlt_properties_set_double( properties, "aspect_ratio", mlt_profile_sar( profile ) );
			mlt_properties_set_int( properties, "meta.media.sample_aspect_num", profile->sample_aspect_num );
			mlt_properties_set_int( properties, "meta.media.sample_aspect_den", profile->sample_aspect_den );
			mlt_properties_set_int( properties, "meta.media.frame_rate_num", profile->frame_rate_num );
			mlt_properties_set_int( properties, "meta.media.frame_rate_den", profile->frame_rate_den );
			mlt_properties_set_int( properties, "width", profile->width );
			mlt_properties_set_int( properties, "meta.media.width", profile->width );
			mlt_properties_set_int( properties, "height", profile->height );
			mlt_properties_set_int( properties, "meta.media.height", profile->height );
			mlt_properties_set_int( properties, "format", ( m_pixel_format == bmdFormat8BitYUV ) ? mlt_image_yuv422 : mlt_image_yuv422p16 );
			mlt_properties_set_int( properties, "colorspace", m_colorspace );
			mlt_properties_set_int( properties, "meta.media.colorspace", m_colorspace );
			mlt_properties_set_int( properties, "audio_frequency", 48000 );
			mlt_properties_set_int( properties, "audio_channels",
				mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "channels" ) );
		}
		else
			mlt_log_warning( getProducer(), "buffer underrun\n" );

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
		mlt_frame frame = NULL;
		struct timeval arrived;

		gettimeofday(&arrived, NULL);

		if( !m_reprio )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( getProducer() );

			if ( mlt_properties_get( properties, "priority" ) )
			{
				int r;
				pthread_t thread;
				pthread_attr_t tattr;
				struct sched_param param;

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
					mlt_log_verbose( getProducer(),
						"VideoInputFrameArrived: pthread_setschedparam returned %d\n", r);
				else
					mlt_log_verbose( getProducer(),
						"VideoInputFrameArrived: param.sched_priority=%d\n", param.sched_priority);
			};

			m_reprio = true;
		};

		if ( mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "preview" ) &&
			mlt_producer_get_speed( getProducer() ) == 0.0 && !mlt_deque_count( m_queue ))
		{
			pthread_cond_broadcast( &m_condition );
			return S_OK;
		}


		// Copy video
		if ( video )
		{
			IDeckLinkTimecode* timecode = 0;

			if ( !( video->GetFlags() & bmdFrameHasNoInputSource ) )
			{
				int vitc_in = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "vitc_in" );
				if ( vitc_in && ( S_OK == video->GetTimecode( bmdTimecodeRP188, &timecode ) ||
					S_OK == video->GetTimecode( bmdTimecodeVITC, &timecode ))  && timecode )
				{
					int vitc = timecode->GetBCD();
					SAFE_RELEASE( timecode );

					mlt_log_verbose( getProducer(),
						"VideoInputFrameArrived: vitc=%.8X vitc_in=%.8X\n", vitc, vitc_in);

					if ( vitc < vitc_in )
					{
						pthread_cond_broadcast( &m_condition );
						return S_OK;
					}

					mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "vitc_in", 0 );
				}

				void *buffer;
				int image_strides[4];
				unsigned char* image_buffers[4];
				mlt_image_format fmt = ( m_pixel_format == bmdFormat8BitYUV ) ? mlt_image_yuv422 : mlt_image_yuv422p16;
				int size = mlt_image_format_size( fmt, video->GetWidth(), video->GetHeight() + m_vancLines, NULL );
				void* image = mlt_pool_alloc( size );

				mlt_image_format_planes( fmt, video->GetWidth(), video->GetHeight() + m_vancLines,
					image, image_buffers, image_strides );

				// Capture VANC
				if ( m_vancLines > 0 )
				{
					IDeckLinkVideoFrameAncillary* vanc = 0;
					if ( video->GetAncillaryData( &vanc ) == S_OK && vanc )
					{
						for ( int i = 1; i < m_vancLines + 1; i++ )
						{
							unsigned char* out[4] = {
								image_buffers[0] + (i - 1) * image_strides[0],
								image_buffers[1] + (i - 1) * image_strides[1],
								image_buffers[2] + (i - 1) * image_strides[2],
								image_buffers[3] + (i - 1) * image_strides[3] };

							if ( vanc->GetBufferForVerticalBlankingLine( i, &buffer ) == S_OK )
								copy_lines
								(
									m_pixel_format, (unsigned char*)buffer, video->GetRowBytes(),
									fmt, out, image_strides,
									video->GetWidth(), 1
								);
							else
							{
								fill_line( fmt, out, image_strides, 0 );
								mlt_log_debug( getProducer(), "failed capture vanc line %d\n", i );
							}
						}
						SAFE_RELEASE(vanc);
					}
				}

				// Capture image
				video->GetBytes( &buffer );
				if ( image && buffer )
				{
					unsigned char* out[4] = {
						image_buffers[0] + m_vancLines * image_strides[0],
						image_buffers[1] + m_vancLines * image_strides[1],
						image_buffers[2] + m_vancLines * image_strides[2],
						image_buffers[3] + m_vancLines * image_strides[3] };

					copy_lines
					(
						m_pixel_format, (unsigned char*)buffer, video->GetRowBytes(),
						fmt, (unsigned char**)out, image_strides,
						video->GetWidth(), video->GetHeight()
					);

					frame = mlt_frame_init( MLT_PRODUCER_SERVICE( getProducer() ) );
					mlt_frame_set_image( frame, (uint8_t*) image, size, mlt_pool_release );
				}
				else if ( image )
				{
					mlt_log_verbose( getProducer(), "no video image\n" );
					mlt_pool_release( image );
				}
			}
			else
			{
				mlt_log_verbose( getProducer(), "no signal\n" );
			}

			// Get timecode
			if ( ( S_OK == video->GetTimecode( bmdTimecodeRP188, &timecode ) ||
				S_OK == video->GetTimecode( bmdTimecodeVITC, &timecode ))  && timecode )
			{
				DLString timecodeString = 0;

				if ( timecode->GetString( &timecodeString ) == S_OK )
				{
					char* s = getCString( timecodeString );
					mlt_properties_set( MLT_FRAME_PROPERTIES( frame ), "meta.attr.vitc.markup", s );
					mlt_log_debug( getProducer(), "timecode %s\n", s );
					freeCString( s );
				}
				freeDLString( timecodeString );
				SAFE_RELEASE( timecode );
			}
		}
		else
		{
			mlt_log_verbose( getProducer(), "no video\n" );
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
				mlt_log_verbose( getProducer(), "no audio samples\n" );
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
			mlt_properties_set_int64( MLT_FRAME_PROPERTIES( frame ), "arrived",
				arrived.tv_sec * 1000000LL + arrived.tv_usec );
			int queueMax = mlt_properties_get_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "buffer" );
			pthread_mutex_lock( &m_mutex );
			if ( mlt_deque_count( m_queue ) < queueMax )
			{
				mlt_deque_push_back( m_queue, frame );
				pthread_cond_broadcast( &m_condition );
			}
			else
			{
				mlt_frame_close( frame );
				mlt_properties_set_int( MLT_PRODUCER_PROPERTIES( getProducer() ), "dropped", ++m_dropped );
				mlt_log_warning( getProducer(), "buffer overrun, frame dropped %d\n", m_dropped );
			}
			pthread_mutex_unlock( &m_mutex );
		}

		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(
			BMDVideoInputFormatChangedEvents events,
			IDeckLinkDisplayMode* mode,
			BMDDetectedVideoInputFormatFlags flags )
	{
		mlt_profile profile = mlt_service_profile( MLT_PRODUCER_SERVICE( getProducer() ) );
		if ( events & bmdVideoInputDisplayModeChanged )
		{
			BMDTimeValue duration;
			BMDTimeScale timescale;
			mode->GetFrameRate( &duration, &timescale );
			profile->width = mode->GetWidth();
			profile->height = mode->GetHeight() + m_vancLines;
			profile->frame_rate_num = timescale;
			profile->frame_rate_den = duration;
			if ( profile->width == 720 )
			{
				if ( profile->height == 576 )
				{
					profile->sample_aspect_num = 16;
					profile->sample_aspect_den = 15;
				}
				else
				{
					profile->sample_aspect_num = 8;
					profile->sample_aspect_den = 9;
				}
				profile->display_aspect_num = 4;
				profile->display_aspect_den = 3;
			}
			else
			{
				profile->sample_aspect_num = 1;
				profile->sample_aspect_den = 1;
				profile->display_aspect_num = 16;
				profile->display_aspect_den = 9;
			}
			free( profile->description );
			profile->description = strdup( "decklink" );
			mlt_log_verbose( getProducer(), "format changed %dx%d %.3f fps\n",
				profile->width, profile->height, (double) profile->frame_rate_num / profile->frame_rate_den );
			m_new_input = profile;
		}
		if ( events & bmdVideoInputFieldDominanceChanged )
		{
			profile->progressive = mode->GetFieldDominance() == bmdProgressiveFrame;
			m_topFieldFirst = mode->GetFieldDominance() == bmdUpperFieldFirst;
			mlt_log_verbose( getProducer(), "field dominance changed prog %d tff %d\n",
				profile->progressive, m_topFieldFirst );
		}
		if ( events & bmdVideoInputColorspaceChanged )
		{
			profile->colorspace = m_colorspace =
				( mode->GetFlags() & bmdDisplayModeColorspaceRec709 ) ? 709 : 601;
			mlt_log_verbose( getProducer(), "colorspace changed %d\n", profile->colorspace );
		}
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
	mlt_position pos = mlt_producer_position( producer );
	mlt_position end = mlt_producer_get_playtime( producer );
	end = ( mlt_producer_get_length( producer ) < end ? mlt_producer_get_length( producer ) : end ) - 1;

	if ( decklink && decklink->m_new_input )
	{
		decklink->m_new_input = NULL;
		decklink->stop();
		decklink->start( decklink->m_new_input );
	}

	// Re-open if needed
	if ( !decklink && pos < end )
	{
		producer->child = decklink = new DeckLinkProducer();
		decklink->setProducer( producer );
		decklink->open(	mlt_properties_get_int( MLT_PRODUCER_PROPERTIES(producer), "resource" ) );
	}

	// Start if needed
	if ( decklink )
	{
		decklink->start( mlt_service_profile( MLT_PRODUCER_SERVICE( producer ) ) );

		// Get the next frame from the decklink object
		if ( ( *frame = decklink->getFrame() ))
		{
			// Add audio and video getters
			mlt_frame_push_audio( *frame, (void*) get_audio );
			mlt_frame_push_get_image( *frame, get_image );
		}
	}
	if ( !*frame )
		*frame = mlt_frame_init( MLT_PRODUCER_SERVICE(producer) );

	// Calculate the next timecode
	mlt_producer_prepare_next( producer );

	// Close DeckLink if at end
	if ( pos >= end && decklink )
	{
		decklink->stop();
		delete decklink;
		producer->child = NULL;
	}

	return 0;
}

static void producer_close( mlt_producer producer )
{
	delete (DeckLinkProducer*) producer->child;
	producer->close = NULL;
	mlt_producer_close( producer );
}

extern "C" {

// Listen for the list_devices property to be set
static void on_property_changed( void*, mlt_properties properties, const char *name )
{
	IDeckLinkIterator* decklinkIterator = NULL;
	IDeckLink* decklink = NULL;
	IDeckLinkInput* decklinkInput = NULL;
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
		if ( decklink->QueryInterface( IID_IDeckLinkInput, (void**) &decklinkInput ) == S_OK )
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
			SAFE_RELEASE( decklinkInput );
		}
		SAFE_RELEASE( decklink );
	}
	SAFE_RELEASE( decklinkIterator );
	mlt_properties_set_int( properties, "devices", i );
}

/** Initialise the producer.
 */

mlt_producer producer_decklink_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	// Allocate the producer
	DeckLinkProducer* decklink = new DeckLinkProducer();
	mlt_producer producer = (mlt_producer) calloc( 1, sizeof( *producer ) );

	// If allocated and initializes
	if ( decklink && !mlt_producer_init( producer, decklink ) )
	{
		if ( decklink->open( arg? atoi( arg ) : 0 ) )
		{
			mlt_properties properties = MLT_PRODUCER_PROPERTIES( producer );

			// Close DeckLink and defer re-open to get_frame
			delete decklink;
			producer->child = NULL;

			// Set callbacks
			producer->close = (mlt_destructor) producer_close;
			producer->get_frame = get_frame;

			// Set properties
			mlt_properties_set( properties, "resource", (arg && strcmp( arg, ""))? arg : "0" );
			mlt_properties_set_int( properties, "channels", 2 );
			mlt_properties_set_int( properties, "buffer", 25 );
			mlt_properties_set_int( properties, "prefill", 25 );

			// These properties effectively make it infinite.
			mlt_properties_set_int( properties, "length", INT_MAX );
			mlt_properties_set_int( properties, "out", INT_MAX - 1 );
			mlt_properties_set( properties, "eof", "loop" );

			mlt_event event = mlt_events_listen( properties, properties, "property-changed", (mlt_listener) on_property_changed );
			mlt_properties_set_data( properties, "list-devices-event", event, 0, NULL, NULL );
		}
	}

	return producer;
}

}
