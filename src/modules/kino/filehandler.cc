/*
* filehandler.cc -- saving DV data into different file formats
* Copyright (C) 2000 Arne Schirmacher <arne@schirmacher.de>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

extern "C" {
#include <framework/mlt_frame.h>
}

#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

using std::cerr;
using std::endl;
using std::ostringstream;
using std::setw;
using std::setfill;

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>

// libdv header files
#ifdef HAVE_LIBDV
#include <libdv/dv.h>
#endif

#include "filehandler.h"
#include "error.h"
#include "riff.h"
#include "avi.h"

FileTracker *FileTracker::instance = NULL;

FileTracker::FileTracker( ) : mode( CAPTURE_MOVIE_APPEND )
{
	cerr << ">> Constructing File Capture tracker" << endl;
}

FileTracker::~FileTracker( )
{
	cerr << ">> Destroying File Capture tracker" << endl;
}

FileTracker &FileTracker::GetInstance( )
{
	if ( instance == NULL )
		instance = new FileTracker();

	return *instance;
}

void FileTracker::SetMode( FileCaptureMode mode )
{
	this->mode = mode;
}

FileCaptureMode FileTracker::GetMode( )
{
	return this->mode;
}

char *FileTracker::Get( int index )
{
	return list[ index ];
}

void FileTracker::Add( const char *file )
{
	if ( this->mode != CAPTURE_IGNORE )
	{
		cerr << ">>>> Registering " << file << " with the tracker" << endl;
		list.push_back( strdup( file ) );
	}
}

unsigned int FileTracker::Size( )
{
	return list.size();
}

void FileTracker::Clear( )
{
	while ( Size() > 0 )
	{
		free( list[ Size() - 1 ] );
		list.pop_back( );
	}
	this->mode = CAPTURE_MOVIE_APPEND;
}

FileHandler::FileHandler() : done( false ), autoSplit( false ), maxFrameCount( 999999 ),
		framesWritten( 0 ), filename( "" )
{
	/* empty body */
	timeStamp = 0;
	everyNthFrame = 0;
	framesToSkip = 0;
	maxFileSize = 0;
}


FileHandler::~FileHandler()
{
	/* empty body */
}


bool FileHandler::GetAutoSplit() const
{
	return autoSplit;
}


bool FileHandler::GetTimeStamp() const
{
	return timeStamp;
}


string FileHandler::GetBaseName() const
{
	return base;
}


string FileHandler::GetExtension() const
{
	return extension;
}


int FileHandler::GetMaxFrameCount() const
{
	return maxFrameCount;
}

off_t FileHandler::GetMaxFileSize() const
{
	return maxFileSize;
}

string FileHandler::GetFilename() const
{
	return filename;
}


void FileHandler::SetAutoSplit( bool flag )
{
	autoSplit = flag;
}


void FileHandler::SetTimeStamp( bool flag )
{
	timeStamp = flag;
}


void FileHandler::SetBaseName( const string& s )
{
	base = s;
}


void FileHandler::SetMaxFrameCount( int count )
{
	assert( count >= 0 );
	maxFrameCount = count;
}


void FileHandler::SetEveryNthFrame( int every )
{
	assert ( every > 0 );

	everyNthFrame = every;
}


void FileHandler::SetMaxFileSize( off_t size )
{
	assert ( size >= 0 );
	maxFileSize = size;
}


#if 0
void FileHandler::SetSampleFrame( const Frame& sample )
{
	/* empty body */
}
#endif

bool FileHandler::Done()
{
	return done;
}

#if 0
bool FileHandler::WriteFrame( const Frame& frame )
{
	static TimeCode prevTimeCode;
	TimeCode timeCode;

	/* If the user wants autosplit, start a new file if a
	   new recording is detected. */
	prevTimeCode.sec = -1;
	frame.GetTimeCode( timeCode );
	int time_diff = timeCode.sec - prevTimeCode.sec;
	bool discontinuity = prevTimeCode.sec != -1 && ( time_diff > 1 || ( time_diff < 0 && time_diff > -59 ) );
	if ( FileIsOpen() && GetAutoSplit() == true && ( frame.IsNewRecording() || discontinuity ) )
	{
		Close();
	}

	if ( FileIsOpen() == false )
	{

		string filename;
		static int counter = 0;

		if ( GetTimeStamp() == true )
		{
			ostringstream sb, sb2;
			struct tm	date;
			string	recDate;

			if ( ! frame.GetRecordingDate( date ) )
			{
				struct timeval tv;
				struct timezone tz;
				gettimeofday( &tv, &tz );
				localtime_r( static_cast< const time_t * >( &tv.tv_sec ), &date );
			}
			sb << setfill( '0' )
			<< setw( 4 ) << date.tm_year + 1900 << '.'
			<< setw( 2 ) << date.tm_mon + 1 << '.'
			<< setw( 2 ) << date.tm_mday << '_'
			<< setw( 2 ) << date.tm_hour << '-'
			<< setw( 2 ) << date.tm_min << '-'
			<< setw( 2 ) << date.tm_sec;
			recDate = sb.str();
			sb2 << GetBaseName() << recDate << GetExtension();
			filename = sb2.str();
			cerr << ">>> Trying " << filename << endl;
		}
		else
		{
			struct stat stats;
			do
			{
				ostringstream sb;
				sb << GetBaseName() << setfill( '0' ) << setw( 3 ) << ++ counter << GetExtension();
				filename = sb.str();
				cerr << ">>> Trying " << filename << endl;
			}
			while ( stat( filename.c_str(), &stats ) == 0 );
		}

		SetSampleFrame( frame );
		if ( Create( filename ) == false )
		{
			cerr << ">>> Error creating file!" << endl;
			return false;
		}
		framesWritten = 0;
		framesToSkip = 0;
	}

	/* write frame */

	if ( framesToSkip == 0 )
	{
		if ( 0 > Write( frame ) )
		{
			cerr << ">>> Error writing frame!" << endl;
			return false;
		}
		framesToSkip = everyNthFrame;
		++framesWritten;
	}
	framesToSkip--;

	/* If the frame count is exceeded, close the current file.
	   If the autosplit flag is set, a new file will be created in the next iteration.
	   If the flag is not set, we are done. */

	if ( ( GetMaxFrameCount() > 0 ) &&
	        ( framesWritten >= GetMaxFrameCount() ) )
	{
		Close();
		done = !GetAutoSplit();
	}

	/* If the file size could be exceeded by another frame, close the current file.
	   If the autosplit flag is set, a new file will be created on the next iteration.
	   If the flag is not set, we are done. */
	/* not exact, but should be good enough to prevent going over. */
	if ( FileIsOpen() )
	{
		AudioInfo info;
		frame.GetAudioInfo( info );
		if ( ( GetFileSize() > 0 ) && ( GetMaxFileSize() > 0 ) &&
		        ( GetFileSize() + frame.GetFrameSize() + info.samples * 4 + 12 )
		        >= GetMaxFileSize() )
		{                     // 12 = sizeof chunk metadata
			Close();
			done = !GetAutoSplit();
		}
	}
    prevTimeCode.sec = timeCode.sec;
	return true;
}
#endif

RawHandler::RawHandler() : fd( -1 )
{
	extension = ".dv";
	numBlocks = 0;
}


RawHandler::~RawHandler()
{
	Close();
}


bool RawHandler::FileIsOpen()
{
	return fd != -1;
}


bool RawHandler::Create( const string& filename )
{
	fd = open( filename.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_NONBLOCK, 0644 );
	if ( fd != -1 )
	{
		FileTracker::GetInstance().Add( filename.c_str() );
		this->filename = filename;
	}
	return ( fd != -1 );
}


#if 0
int RawHandler::Write( const Frame& frame )
{
	int result = write( fd, frame.data, frame.GetFrameSize() );
	return result;
}
#endif

int RawHandler::Close()
{
	if ( fd != -1 )
	{
		close( fd );
		fd = -1;
	}
	return 0;
}


off_t RawHandler::GetFileSize()
{
	struct stat file_status;
	fstat( fd, &file_status );
	return file_status.st_size;
}

int RawHandler::GetTotalFrames()
{
	return GetFileSize() / ( 480 * numBlocks );
}


bool RawHandler::Open( const char *s )
{
	unsigned char data[ 4 ];
	assert( fd == -1 );
	fd = open( s, O_RDONLY | O_NONBLOCK );
	if ( fd < 0 )
		return false;
	if ( read( fd, data, 4 ) < 0 )
		return false;
	if ( lseek( fd, 0, SEEK_SET ) < 0 )
		return false;
	numBlocks = ( ( data[ 3 ] & 0x80 ) == 0 ) ? 250 : 300;
	filename = s;
	return true;

}

int RawHandler::GetFrame( uint8_t *data, int frameNum )
{
	assert( fd != -1 );
	int size = 480 * numBlocks;
	if ( frameNum < 0 )
		return -1;
	off_t offset = ( ( off_t ) frameNum * ( off_t ) size );
	fail_if( lseek( fd, offset, SEEK_SET ) == ( off_t ) - 1 );
	if ( read( fd, data, size ) > 0 )
		return 0;
	else
		return -1;
}


/***************************************************************************/


AVIHandler::AVIHandler( int format ) : avi( NULL ), aviFormat( format ), isOpenDML( false ),
		fccHandler( make_fourcc( "dvsd" ) ), channels( 2 ), isFullyInitialized( false ),
		audioBuffer( NULL )
{
	extension = ".avi";
	for ( int c = 0; c < 4; c++ )
		audioChannels[ c ] = NULL;
	memset( &dvinfo, 0, sizeof( dvinfo ) );
}


AVIHandler::~AVIHandler()
{
	delete audioBuffer;
	audioBuffer = NULL;

	for ( int c = 0; c < 4; c++ )
	{
		delete audioChannels[ c ];
		audioChannels[ c ] = NULL;
	}

	delete avi;
}

#if 0
void AVIHandler::SetSampleFrame( const Frame& sample )
{
	Pack pack;
	sample.GetAudioInfo( audioInfo );
	sample.GetVideoInfo( videoInfo );

	sample.GetAAUXPack( 0x50, pack );
	dvinfo.dwDVAAuxSrc = *( DWORD* ) ( pack.data + 1 );
	sample.GetAAUXPack( 0x51, pack );
	dvinfo.dwDVAAuxCtl = *( DWORD* ) ( pack.data + 1 );

	sample.GetAAUXPack( 0x52, pack );
	dvinfo.dwDVAAuxSrc1 = *( DWORD* ) ( pack.data + 1 );
	sample.GetAAUXPack( 0x53, pack );
	dvinfo.dwDVAAuxCtl1 = *( DWORD* ) ( pack.data + 1 );

	sample.GetVAUXPack( 0x60, pack );
	dvinfo.dwDVVAuxSrc = *( DWORD* ) ( pack.data + 1 );
	sample.GetVAUXPack( 0x61, pack );
	dvinfo.dwDVVAuxCtl = *( DWORD* ) ( pack.data + 1 );

#ifdef WITH_LIBDV

	if ( sample.decoder->std == e_dv_std_smpte_314m )
		fccHandler = make_fourcc( "dv25" );
#endif
}
#endif

bool AVIHandler::FileIsOpen()
{
	return avi != NULL;
}


bool AVIHandler::Create( const string& filename )
{
	assert( avi == NULL );

	switch ( aviFormat )
	{

	case AVI_DV1_FORMAT:
		fail_null( avi = new AVI1File );
		if ( !avi || avi->Create( filename.c_str() ) == false )
			return false;
		//avi->Init( videoInfo.isPAL ? AVI_PAL : AVI_NTSC, audioInfo.frequency, AVI_LARGE_INDEX );
		break;

	case AVI_DV2_FORMAT:
		fail_null( avi = new AVI2File );
		if ( !avi || avi->Create( filename.c_str() ) == false )
			return false;
		//if ( GetOpenDML() )
			//avi->Init( videoInfo.isPAL ? AVI_PAL : AVI_NTSC, audioInfo.frequency,
			           //( AVI_SMALL_INDEX | AVI_LARGE_INDEX ) );
		//else
			//avi->Init( videoInfo.isPAL ? AVI_PAL : AVI_NTSC, audioInfo.frequency,
			           //( AVI_SMALL_INDEX ) );
		break;

	default:
		assert( aviFormat == AVI_DV1_FORMAT || aviFormat == AVI_DV2_FORMAT );
	}

	avi->setDVINFO( dvinfo );
	avi->setFccHandler( make_fourcc( "iavs" ), fccHandler );
	avi->setFccHandler( make_fourcc( "vids" ), fccHandler );
	this->filename = filename;
	FileTracker::GetInstance().Add( filename.c_str() );
	return ( avi != NULL );
}

#if 0
int AVIHandler::Write( const Frame& frame )
{
	assert( avi != NULL );
	try
	{
		return avi->WriteFrame( frame ) ? 0 : -1;
	}
	catch (...)
	{
		return -1;
	}
}
#endif

int AVIHandler::Close()
{
	if ( avi != NULL )
	{
		avi->WriteRIFF();
		delete avi;
		avi = NULL;
	}
	if ( audioBuffer != NULL )
	{
		delete audioBuffer;
		audioBuffer = NULL;
	}
	for ( int c = 0; c < 4; c++ )
	{
		if ( audioChannels[ c ] != NULL )
		{
			delete audioChannels[ c ];
			audioChannels[ c ] = NULL;
		}
	}
	isFullyInitialized = false;
	return 0;
}

off_t AVIHandler::GetFileSize()
{
	return avi->GetFileSize();
}

int AVIHandler::GetTotalFrames()
{
	return avi->GetTotalFrames();
}


bool AVIHandler::Open( const char *s )
{
	assert( avi == NULL );
	fail_null( avi = new AVI1File );
	if ( avi->Open( s ) )
	{
		avi->ParseRIFF();
		if ( ! (
		            avi->verifyStreamFormat( make_fourcc( "dvsd" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "DVSD" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "dvcs" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "DVCS" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "dvcp" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "DVCP" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "CDVC" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "cdvc" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "DV25" ) ) ||
		            avi->verifyStreamFormat( make_fourcc( "dv25" ) ) ) )
			return false;
		avi->ReadIndex();
		if ( avi->verifyStream( make_fourcc( "auds" ) ) )
			aviFormat = AVI_DV2_FORMAT;
		else
			aviFormat = AVI_DV1_FORMAT;
		isOpenDML = avi->isOpenDML();
		filename = s;
		return true;
	}
	else
		return false;

}

int AVIHandler::GetFrame( uint8_t *data, int frameNum )
{
	int result = avi->GetDVFrame( data, frameNum );
#if 0
	if ( result == 0 )
	{
		/* get the audio from the audio stream, if available */
		if ( aviFormat == AVI_DV2_FORMAT )
		{
			WAVEFORMATEX wav;

			if ( ! isFullyInitialized && 
				 avi->getStreamFormat( ( void* ) &wav, make_fourcc( "auds" ) ) )
			{
				if ( channels > 0 && channels < 5 )
				{
					// Allocate interleaved audio buffer
					audioBuffer = new int16_t[ 2 * DV_AUDIO_MAX_SAMPLES * channels ];

					// Allocate non-interleaved audio buffers
					for ( int c = 0; c < channels; c++ )
						audioChannels[ c ] = new int16_t[ 2 * DV_AUDIO_MAX_SAMPLES ];
					
					// Get the audio parameters from AVI for subsequent calls to method
					audioInfo.channels = wav.nChannels;
					audioInfo.frequency = wav.nSamplesPerSec;

					// Skip initialization on subsequent calls to method
					isFullyInitialized = true;
					cerr << ">>> using audio from separate AVI audio stream" << endl;
				}
			}

			// Get the frame from AVI
			int n = avi->getFrame( audioBuffer, frameNum, make_fourcc( "01wb" ) );
			if ( n > 0 )
			{
				// Temporary pointer to audio scratch buffer
				int16_t * s = audioBuffer;

				// Determine samples in this frame
				audioInfo.samples = n / audioInfo.channels / sizeof( int16_t );
				
				// Convert interleaved audio into non-interleaved
				for ( int n = 0; n < audioInfo.samples; ++n )
					for ( int i = 0; i < audioInfo.channels; i++ )
						audioChannels[ i ][ n ] = *s++;

				// Write interleaved audio into frame
				frame.EncodeAudio( audioInfo, audioChannels );
			}
		}

		// Parse important metadata in DV bitstream
		frame.ExtractHeader();
	}
#endif
	return result;
}


void AVIHandler::SetOpenDML( bool flag )
{
	isOpenDML = flag;
}


bool AVIHandler::GetOpenDML() const
{
	return isOpenDML;
}


/***************************************************************************/

#ifdef HAVE_LIBQUICKTIME

#ifndef HAVE_LIBDV
#define DV_AUDIO_MAX_SAMPLES 1944
#endif

// Missing fourcc's in libquicktime (allows compilation)
#ifndef QUICKTIME_DV_AVID
#define QUICKTIME_DV_AVID "AVdv"
#endif

#ifndef QUICKTIME_DV_AVID_A
#define QUICKTIME_DV_AVID_A "dvcp"
#endif

#ifndef QUICKTIME_DVCPRO
#define QUICKTIME_DVCPRO "dvpp"
#endif

QtHandler::QtHandler() : fd( NULL )
{
	extension = ".mov";
	Init();
}


QtHandler::~QtHandler()
{
	Close();
}

void QtHandler::Init()
{
	if ( fd != NULL )
		Close();

	fd = NULL;
	samplingRate = 0;
	samplesPerBuffer = 0;
	channels = 2;
	audioBuffer = NULL;
	audioChannelBuffer = NULL;
	isFullyInitialized = false;
}


bool QtHandler::FileIsOpen()
{
	return fd != NULL;
}


bool QtHandler::Create( const string& filename )
{
	Init();

	if ( open( filename.c_str(), O_CREAT | O_TRUNC | O_RDWR | O_NONBLOCK, 0644 ) != -1 )
	{
		fd = quicktime_open( const_cast<char*>( filename.c_str() ), 0, 1 );
		if ( fd != NULL )
			FileTracker::GetInstance().Add( filename.c_str() );
	}
	else
		return false;
	this->filename = filename;
	return true;
}

void QtHandler::AllocateAudioBuffers()
{
	if ( channels > 0 && channels < 5 )
	{
		audioBufferSize = DV_AUDIO_MAX_SAMPLES * 2;
		audioBuffer = new int16_t[ audioBufferSize * channels ];

		audioChannelBuffer = new short int * [ channels ];
		for ( int c = 0; c < channels; c++ )
			audioChannelBuffer[ c ] = new short int[ audioBufferSize ];
		isFullyInitialized = true;
	}
}

inline void QtHandler::DeinterlaceStereo16( void* pInput, int iBytes,
        void* pLOutput, void* pROutput )
{
	short int * piSampleInput = ( short int* ) pInput;
	short int* piSampleLOutput = ( short int* ) pLOutput;
	short int* piSampleROutput = ( short int* ) pROutput;

	while ( ( char* ) piSampleInput < ( ( char* ) pInput + iBytes ) )
	{
		*piSampleLOutput++ = *piSampleInput++;
		*piSampleROutput++ = *piSampleInput++;
	}
}

#if 0
int QtHandler::Write( const Frame& frame )
{
	if ( ! isFullyInitialized )
	{
		AudioInfo audio;

		if ( frame.GetAudioInfo( audio ) )
		{
			channels = 2;
			quicktime_set_audio( fd, channels, audio.frequency, 16, QUICKTIME_TWOS );
		}
		else
		{
			channels = 0;
		}

		quicktime_set_video( fd, 1, 720, frame.IsPAL() ? 576 : 480,
		                     frame.GetFrameRate(), QUICKTIME_DV );
		AllocateAudioBuffers();
	}

	int result = quicktime_write_frame( fd, const_cast<unsigned char*>( frame.data ),
	                                    frame.GetFrameSize(), 0 );

	if ( channels > 0 )
	{
		AudioInfo audio;
		if ( frame.GetAudioInfo( audio ) && ( unsigned int ) audio.samples < audioBufferSize )
		{
			long bytesRead = frame.ExtractAudio( audioBuffer );

			DeinterlaceStereo16( audioBuffer, bytesRead,
			                     audioChannelBuffer[ 0 ],
			                     audioChannelBuffer[ 1 ] );

			quicktime_encode_audio( fd, audioChannelBuffer, NULL, audio.samples );
		}
	}
	return result;
}
#endif

int QtHandler::Close()
{
	if ( fd != NULL )
	{
		quicktime_close( fd );
		fd = NULL;
	}
	if ( audioBuffer != NULL )
	{
		delete audioBuffer;
		audioBuffer = NULL;
	}
	if ( audioChannelBuffer != NULL )
	{
		for ( int c = 0; c < channels; c++ )
			delete audioChannelBuffer[ c ];
		delete audioChannelBuffer;
		audioChannelBuffer = NULL;
	}
	return 0;
}


off_t QtHandler::GetFileSize()
{
	struct stat file_status;
	stat( filename.c_str(), &file_status );
	return file_status.st_size;
}


int QtHandler::GetTotalFrames()
{
	return ( int ) quicktime_video_length( fd, 0 );
}


bool QtHandler::Open( const char *s )
{
	Init();

	fd = quicktime_open( s, 1, 0 );
	if ( fd == NULL )
	{
		fprintf( stderr, "Error opening: %s\n", s );
		return false;
	}

	if ( quicktime_has_video( fd ) <= 0 )
	{
		fprintf( stderr, "There must be at least one video track in the input file (%s).\n",
		         s );
		Close();
		return false;
	}
	char * fcc = quicktime_video_compressor( fd, 0 );
	if ( strncmp( fcc, QUICKTIME_DV, 4 ) != 0 &&
	     strncmp( fcc, QUICKTIME_DV_AVID, 4 ) != 0 &&
	     strncmp( fcc, QUICKTIME_DV_AVID_A, 4 ) != 0 && 
	     strncmp( fcc, QUICKTIME_DVCPRO, 4 ) != 0 )
	{
		Close();
		return false;
	}
	if ( quicktime_has_audio( fd ) )
		channels = quicktime_track_channels( fd, 0 );
	filename = s;
	return true;
}

int QtHandler::GetFrame( uint8_t *data, int frameNum )
{
	assert( fd != NULL );

	quicktime_set_video_position( fd, frameNum, 0 );
	quicktime_read_frame( fd, data, 0 );

#ifdef HAVE_LIBDV
	if ( quicktime_has_audio( fd ) )
	{
		if ( ! isFullyInitialized )
			AllocateAudioBuffers();

		// Fetch the frequency of the audio track and calc number of samples needed
		int frequency = quicktime_sample_rate( fd, 0 );
		float fps = ( data[ 3 ] & 0x80 ) ? 25.0f : 29.97f;
		int samples = mlt_sample_calculator( fps, frequency, frameNum );
		int64_t seek = mlt_sample_calculator_to_now( fps, frequency, frameNum );

		// Obtain a dv encoder and initialise it with minimal info
		dv_encoder_t *encoder = dv_encoder_new( 0, 0, 0 );
		encoder->isPAL = ( data[ 3 ] & 0x80 );
		encoder->samples_this_frame = samples;

		// Seek to the calculated position and decode
		quicktime_set_audio_position( fd, seek, 0 );
		lqt_decode_audio( fd, audioChannelBuffer, NULL, (long) samples );

		// Encode the audio on the frame and done
		dv_encode_full_audio( encoder, audioChannelBuffer, channels, frequency, data );
		dv_encoder_free( encoder );
	}
#endif

	return 0;
}
#endif
