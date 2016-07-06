/*
* avi.cc library for AVI file format i/o
* Copyright (C) 2000 - 2002 Arne Schirmacher <arne@schirmacher.de>
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

// C++ includes

#include <cstring>
#include <iostream>
#include <iomanip>

using std::cout;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;
using std::endl;

// C includes

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

// local includes

#include "error.h"
#include "riff.h"
#include "avi.h"

#define PADDING_SIZE (512)
#define PADDING_1GB (0x40000000)
#define IX00_INDEX_SIZE (4028)

#define AVIF_HASINDEX 0x00000010
#define AVIF_MUSTUSEINDEX 0x00000020
#define AVIF_TRUSTCKTYPE 0x00000800
#define AVIF_ISINTERLEAVED 0x00000100
#define AVIF_WASCAPTUREFILE 0x00010000
#define AVIF_COPYRIGHTED 0x00020000


//static char g_zeroes[ PADDING_SIZE ];

/** The constructor
 
    \todo mainHdr not initialized
    \todo add checking for NULL pointers
 
*/

AVIFile::AVIFile() : RIFFFile(),
		idx1( NULL ), file_list( -1 ), riff_list( -1 ),
		hdrl_list( -1 ), avih_chunk( -1 ), movi_list( -1 ), junk_chunk( -1 ), idx1_chunk( -1 ),
		index_type( -1 ), current_ix00( -1 ), odml_list( -1 ), dmlh_chunk( -1 ), isUpdateIdx1( true )
{
	// cerr << "0x" << hex << (long)this << dec << " AVIFile::AVIFile() : RIFFFile(), ..." << endl;

	for ( int i = 0; i < 2; ++i )
	{
		indx[ i ] = new AVISuperIndex;
		memset( indx[ i ], 0, sizeof( AVISuperIndex ) );
		ix[ i ] = new AVIStdIndex;
		memset( ix[ i ], 0, sizeof( AVIStdIndex ) );
		indx_chunk[ i ] = -1;
		ix_chunk[ i ] = -1;
		strl_list[ i ] = -1;
		strh_chunk[ i ] = -1;
		strf_chunk[ i ] = -1;
	}
	idx1 = new AVISimpleIndex;
	memset( idx1, 0, sizeof( AVISimpleIndex ) );
	memset( dmlh, 0, sizeof( dmlh ) );
	memset( &mainHdr, 0, sizeof( mainHdr ) );
	memset( &streamHdr, 0, sizeof( streamHdr ) );
}


/** The copy constructor
 
    \todo add checking for NULL pointers
 
*/

AVIFile::AVIFile( const AVIFile& avi ) : RIFFFile( avi )
{
	// cerr << "0x" << hex << (long)this << dec << " 0x" << hex << (long)&avi << dec << " AVIFile::AVIFile(const AVIFile& avi) : RIFFFile(avi)" << endl;

	mainHdr = avi.mainHdr;
	idx1 = new AVISimpleIndex;
	*idx1 = *avi.idx1;
	file_list = avi.file_list;
	riff_list = avi.riff_list;
	hdrl_list = avi.hdrl_list;
	avih_chunk = avi.avih_chunk;
	movi_list = avi.movi_list;
	junk_chunk = avi.junk_chunk;
	idx1_chunk = avi.idx1_chunk;

	for ( int i = 0; i < 2; ++i )
	{
		indx[ i ] = new AVISuperIndex;
		*indx[ i ] = *avi.indx[ i ];
		ix[ i ] = new AVIStdIndex;
		*ix[ i ] = *avi.ix[ i ];
		indx_chunk[ i ] = avi.indx_chunk[ i ];
		ix_chunk[ i ] = avi.ix_chunk[ i ];
		strl_list[ i ] = avi.strl_list[ i ];
		strh_chunk[ i ] = avi.strh_chunk[ i ];
		strf_chunk[ i ] = avi.strf_chunk[ i ];
	}

	index_type = avi.index_type;
	current_ix00 = avi.current_ix00;

	for ( int i = 0; i < 62; ++i )
		dmlh[ i ] = avi.dmlh[ i ];

	isUpdateIdx1 = avi.isUpdateIdx1;

	odml_list = 0;
	dmlh_chunk = 0;
	memset( &streamHdr, 0, sizeof( streamHdr ) );
}


/** The assignment operator
 
*/

AVIFile& AVIFile::operator=( const AVIFile& avi )
{
	// cerr << "0x" << hex << (long)this << dec << " 0x" << hex << (long)&avi << dec << " AVIFile& AVIFile::operator=(const AVIFile& avi)" << endl;

	if ( this != &avi )
	{
		RIFFFile::operator=( avi );
		mainHdr = avi.mainHdr;
		*idx1 = *avi.idx1;
		file_list = avi.file_list;
		riff_list = avi.riff_list;
		hdrl_list = avi.hdrl_list;
		avih_chunk = avi.avih_chunk;
		movi_list = avi.movi_list;
		junk_chunk = avi.junk_chunk;
		idx1_chunk = avi.idx1_chunk;

		for ( int i = 0; i < 2; ++i )
		{
			*indx[ i ] = *avi.indx[ i ];
			*ix[ i ] = *avi.ix[ i ];
			indx_chunk[ i ] = avi.indx_chunk[ i ];
			ix_chunk[ i ] = avi.ix_chunk[ i ];
			strl_list[ i ] = avi.strl_list[ i ];
			strh_chunk[ i ] = avi.strh_chunk[ i ];
			strf_chunk[ i ] = avi.strf_chunk[ i ];
		}

		index_type = avi.index_type;
		current_ix00 = avi.current_ix00;

		for ( int i = 0; i < 62; ++i )
			dmlh[ i ] = avi.dmlh[ i ];

		isUpdateIdx1 = avi.isUpdateIdx1;
	}
	return *this;
}


/** The destructor
 
*/

AVIFile::~AVIFile()
{
	// cerr << "0x" << hex << (long)this << dec << " AVIFile::~AVIFile()" << endl;

	for ( int i = 0; i < 2; ++i )
	{
		delete ix[ i ];
		delete indx[ i ];
	}
	delete idx1;
}

/** Initialize the AVI structure to its initial state, either for PAL or NTSC format
 
    Initialize the AVIFile attributes: mainHdr, indx, ix00, idx1
 
    \todo consolidate AVIFile::Init, AVI1File::Init, AVI2File::Init. They are somewhat redundant.
    \param format pass AVI_PAL or AVI_NTSC
    \param sampleFrequency the sample frequency of the audio content
    \param indexType pass AVI_SMALL_INDEX or AVI_LARGE_INDEX
 
*/

void AVIFile::Init( int format, int sampleFrequency, int indexType )
{
	int i, j;

	assert( ( format == AVI_PAL ) || ( format == AVI_NTSC ) );

	index_type = indexType;

	switch ( format )
	{
	case AVI_PAL:
		mainHdr.dwMicroSecPerFrame = 40000;
		mainHdr.dwSuggestedBufferSize = 144008;
		break;

	case AVI_NTSC:
		mainHdr.dwMicroSecPerFrame = 33366;
		mainHdr.dwSuggestedBufferSize = 120008;
		break;

	default:   /* no default allowed */
		assert( 0 );
		break;
	}

	/* Initialize the 'avih' chunk */

	mainHdr.dwMaxBytesPerSec = 3600000 + sampleFrequency * 4;
	mainHdr.dwPaddingGranularity = PADDING_SIZE;
	mainHdr.dwFlags = AVIF_TRUSTCKTYPE;
	if ( indexType & AVI_SMALL_INDEX )
		mainHdr.dwFlags |= AVIF_HASINDEX;
	mainHdr.dwTotalFrames = 0;
	mainHdr.dwInitialFrames = 0;
	mainHdr.dwStreams = 1;
	mainHdr.dwWidth = 0;
	mainHdr.dwHeight = 0;
	mainHdr.dwReserved[ 0 ] = 0;
	mainHdr.dwReserved[ 1 ] = 0;
	mainHdr.dwReserved[ 2 ] = 0;
	mainHdr.dwReserved[ 3 ] = 0;

	/* Initialize the 'idx1' chunk */

	for ( int i = 0; i < 8000; ++i )
	{
		idx1->aIndex[ i ].dwChunkId = 0;
		idx1->aIndex[ i ].dwFlags = 0;
		idx1->aIndex[ i ].dwOffset = 0;
		idx1->aIndex[ i ].dwSize = 0;
	}
	idx1->nEntriesInUse = 0;

	/* Initialize the 'indx' chunk */

	for ( i = 0; i < 2; ++i )
	{
		indx[ i ] ->wLongsPerEntry = 4;
		indx[ i ] ->bIndexSubType = 0;
		indx[ i ] ->bIndexType = KINO_AVI_INDEX_OF_INDEXES;
		indx[ i ] ->nEntriesInUse = 0;
		indx[ i ] ->dwReserved[ 0 ] = 0;
		indx[ i ] ->dwReserved[ 1 ] = 0;
		indx[ i ] ->dwReserved[ 2 ] = 0;
		for ( j = 0; j < 2014; ++j )
		{
			indx[ i ] ->aIndex[ j ].qwOffset = 0;
			indx[ i ] ->aIndex[ j ].dwSize = 0;
			indx[ i ] ->aIndex[ j ].dwDuration = 0;
		}
	}

	/* The ix00 and ix01 chunk will be added dynamically in avi_write_frame
	          as needed */

	/* Initialize the 'dmlh' chunk. I have no clue what this means
	   though */

	for ( i = 0; i < 62; ++i )
		dmlh[ i ] = 0;
	//dmlh[0] = -1;            /* frame count + 1? */

}


/** Find position and size of a given frame in the file
 
    Depending on which index is available, search one of them to
    find position and frame size
 
    \todo the size parameter is redundant. All frames have the same size, which is also in the mainHdr.
    \todo all index related operations should be isolated 
    \param offset the file offset to the start of the frame
    \param size the size of the frame
    \param frameNum the number of the frame we wish to find
    \return 0 if the frame could be found, -1 otherwise
*/

int AVIFile::GetDVFrameInfo( off_t &offset, int &size, int frameNum )
{
	switch ( index_type )
	{
	case AVI_LARGE_INDEX:

		/* find relevant index in indx0 */

		int i;

		for ( i = 0; frameNum >= indx[ 0 ] ->aIndex[ i ].dwDuration; frameNum -= indx[ 0 ] ->aIndex[ i ].dwDuration, ++i )
			;

		if ( i != current_ix00 )
		{
			fail_if( lseek( fd, indx[ 0 ] ->aIndex[ i ].qwOffset + RIFF_HEADERSIZE, SEEK_SET ) == ( off_t ) - 1 );
			fail_neg( read( fd, ix[ 0 ], indx[ 0 ] ->aIndex[ i ].dwSize - RIFF_HEADERSIZE ) );
			current_ix00 = i;
		}

		if ( frameNum < ix[ 0 ] ->nEntriesInUse )
		{
			offset = ix[ 0 ] ->qwBaseOffset + ix[ 0 ] ->aIndex[ frameNum ].dwOffset;
			size = ix[ 0 ] ->aIndex[ frameNum ].dwSize;
			return 0;
		}
		else
			return -1;
		break;

	case AVI_SMALL_INDEX:
		int index = -1;
		int frameNumIndex = 0;
		for ( int i = 0; i < idx1->nEntriesInUse; ++i )
		{
			FOURCC chunkID1 = make_fourcc( "00dc" );
			FOURCC chunkID2 = make_fourcc( "00db" );
			if ( idx1->aIndex[ i ].dwChunkId == chunkID1 ||
			        idx1->aIndex[ i ].dwChunkId == chunkID2 )
			{
				if ( frameNumIndex == frameNum )
				{
					index = i;
					break;
				}
				++frameNumIndex;
			}
		}
		if ( index != -1 )
		{
			// compatibility check for broken dvgrab dv2 format
			if ( idx1->aIndex[ 0 ].dwOffset > GetDirectoryEntry( movi_list ).offset )
			{
				offset = idx1->aIndex[ index ].dwOffset + RIFF_HEADERSIZE;
			}
			else
			{
				// new, correct dv2 format
				offset = idx1->aIndex[ index ].dwOffset + RIFF_HEADERSIZE + GetDirectoryEntry( movi_list ).offset;
			}
			size = idx1->aIndex[ index ].dwSize;
			return 0;
		}
		else
			return -1;
		break;
	}
	return -1;
}

/** Find position and size of a given frame in the file
 
    Depending on which index is available, search one of them to
    find position and frame size
 
    \todo the size parameter is redundant. All frames have the same size, which is also in the mainHdr.
    \todo all index related operations should be isolated 
    \param offset the file offset to the start of the frame
    \param size the size of the frame
    \param frameNum the number of the frame we wish to find
	\param chunkID the ID of the type of chunk we want
    \return 0 if the frame could be found, -1 otherwise
*/

int AVIFile::GetFrameInfo( off_t &offset, int &size, int frameNum, FOURCC chunkID )
{
	if ( index_type & AVI_LARGE_INDEX )
	{
		int i;

		for ( i = 0; frameNum >= indx[ 0 ] ->aIndex[ i ].dwDuration; frameNum -= indx[ 0 ] ->aIndex[ i ].dwDuration, ++i )
			;

		if ( i != current_ix00 )
		{
			fail_if( lseek( fd, indx[ 0 ] ->aIndex[ i ].qwOffset + RIFF_HEADERSIZE, SEEK_SET ) == ( off_t ) - 1 );
			fail_neg( read( fd, ix[ 0 ], indx[ 0 ] ->aIndex[ i ].dwSize - RIFF_HEADERSIZE ) );
			current_ix00 = i;
		}

		if ( frameNum < ix[ 0 ] ->nEntriesInUse )
		{
			if ( ( FOURCC ) ix[ 0 ] ->dwChunkId == chunkID )
			{
				offset = ix[ 0 ] ->qwBaseOffset + ix[ 0 ] ->aIndex[ frameNum ].dwOffset;
				size = ix[ 0 ] ->aIndex[ frameNum ].dwSize;
				return 0;
			}
		}
	}
	if ( index_type & AVI_SMALL_INDEX )
	{
		int index = -1;
		int frameNumIndex = 0;
		for ( int i = 0; i < idx1->nEntriesInUse; ++i )
		{
			if ( idx1->aIndex[ i ].dwChunkId == chunkID )
			{
				if ( frameNumIndex == frameNum )
				{
					index = i;
					break;
				}
				++frameNumIndex;
			}
		}
		if ( index != -1 )
		{
			// compatibility check for broken dvgrab dv2 format
			if ( idx1->aIndex[ 0 ].dwOffset > GetDirectoryEntry( movi_list ).offset )
			{
				offset = idx1->aIndex[ index ].dwOffset + RIFF_HEADERSIZE;
			}
			else
			{
				// new, correct dv2 format
				offset = idx1->aIndex[ index ].dwOffset + RIFF_HEADERSIZE + GetDirectoryEntry( movi_list ).offset;
			}
			size = idx1->aIndex[ index ].dwSize;
			return 0;
		}
	}
	return -1;
}

/** Read in a frame
 
    \todo we actually don't need the frame here, we could use just a void pointer
    \param frame a reference to the frame object that will receive the frame data
    \param frameNum the frame number to read
    \return 0 if the frame could be read, -1 otherwise
*/

int AVIFile::GetDVFrame( uint8_t *data, int frameNum )
{
	off_t	offset;
	int	size;

	if ( GetDVFrameInfo( offset, size, frameNum ) != 0 || size < 0 )
		return -1;
	pthread_mutex_lock( &file_mutex );
	fail_if( lseek( fd, offset, SEEK_SET ) == ( off_t ) - 1 );
	fail_neg( read( fd, data, size ) );
	pthread_mutex_unlock( &file_mutex );

	return 0;
}

/** Read in a frame
 
    \param data a pointer to the audio buffer
    \param frameNum the frame number to read
	\param chunkID the ID of the type of chunk we want
    \return the size the of the frame data, 0 if could not be read
*/

int AVIFile::getFrame( void *data, int frameNum, FOURCC chunkID )
{
	off_t offset;
	int	size;

	if ( GetFrameInfo( offset, size, frameNum, chunkID ) != 0 )
		return 0;
	fail_if( lseek( fd, offset, SEEK_SET ) == ( off_t ) - 1 );
	fail_neg( read( fd, data, size ) );

	return size;
}

int AVIFile::GetTotalFrames() const
{
	return mainHdr.dwTotalFrames;
}


/** prints out a directory entry in text form
 
    Every subclass of RIFFFile is supposed to override this function
    and to implement it for the entry types it knows about. For all
    other entry types it should call its parent::PrintDirectoryData.
 
    \todo use 64 bit routines
    \param entry the entry to print
*/

void AVIFile::PrintDirectoryEntryData( const RIFFDirEntry &entry ) const
{
	static FOURCC lastStreamType = make_fourcc( "    " );

	if ( entry.type == make_fourcc( "avih" ) )
	{

		int i;
		MainAVIHeader main_avi_header;

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, &main_avi_header, sizeof( MainAVIHeader ) ) );

		cout << "    dwMicroSecPerFrame:    " << ( int ) main_avi_header.dwMicroSecPerFrame << endl
		<< "    dwMaxBytesPerSec:      " << ( int ) main_avi_header.dwMaxBytesPerSec << endl
		<< "    dwPaddingGranularity:  " << ( int ) main_avi_header.dwPaddingGranularity << endl
		<< "    dwFlags:               " << ( int ) main_avi_header.dwFlags << endl
		<< "    dwTotalFrames:         " << ( int ) main_avi_header.dwTotalFrames << endl
		<< "    dwInitialFrames:       " << ( int ) main_avi_header.dwInitialFrames << endl
		<< "    dwStreams:             " << ( int ) main_avi_header.dwStreams << endl
		<< "    dwSuggestedBufferSize: " << ( int ) main_avi_header.dwSuggestedBufferSize << endl
		<< "    dwWidth:               " << ( int ) main_avi_header.dwWidth << endl
		<< "    dwHeight:              " << ( int ) main_avi_header.dwHeight << endl;
		for ( i = 0; i < 4; ++i )
			cout << "    dwReserved[" << i << "]:        " << ( int ) main_avi_header.dwReserved[ i ] << endl;

	}
	else if ( entry.type == make_fourcc( "strh" ) )
	{

		AVIStreamHeader	avi_stream_header;

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, &avi_stream_header, sizeof( AVIStreamHeader ) ) );

		lastStreamType = avi_stream_header.fccType;

		cout << "    fccType:         '"
		<< ((char *)&avi_stream_header.fccType)[0]
		<< ((char *)&avi_stream_header.fccType)[1]
		<< ((char *)&avi_stream_header.fccType)[2]
		<< ((char *)&avi_stream_header.fccType)[3]
		<< '\'' << endl
		<< "    fccHandler:      '"
		<< ((char *)&avi_stream_header.fccHandler)[0]
		<< ((char *)&avi_stream_header.fccHandler)[1]
		<< ((char *)&avi_stream_header.fccHandler)[2]
		<< ((char *)&avi_stream_header.fccHandler)[3]
		<< '\'' << endl
		<< "    dwFlags:         " << ( int ) avi_stream_header.dwFlags << endl
		<< "    wPriority:       " << ( int ) avi_stream_header.wPriority << endl
		<< "    wLanguage:       " << ( int ) avi_stream_header.wLanguage << endl
		<< "    dwInitialFrames: " << ( int ) avi_stream_header.dwInitialFrames << endl
		<< "    dwScale:         " << ( int ) avi_stream_header.dwScale << endl
		<< "    dwRate:          " << ( int ) avi_stream_header.dwRate << endl
		<< "    dwLength:        " << ( int ) avi_stream_header.dwLength << endl
		<< "    dwQuality:       " << ( int ) avi_stream_header.dwQuality << endl
		<< "    dwSampleSize:    " << ( int ) avi_stream_header.dwSampleSize << endl;

	}
	else if ( entry.type == make_fourcc( "indx" ) )
	{

		int i;
		AVISuperIndex avi_super_index;

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, &avi_super_index, sizeof( AVISuperIndex ) ) );

		cout << "    wLongsPerEntry: " << ( int ) avi_super_index.wLongsPerEntry
		<< endl
		<< "    bIndexSubType:  " << ( int ) avi_super_index.bIndexSubType << endl
		<< "    bIndexType:     " << ( int ) avi_super_index.bIndexType << endl
		<< "    nEntriesInUse:  " << ( int ) avi_super_index.nEntriesInUse << endl
		<< "    dwChunkId:      '"
		<< ((char *)&avi_super_index.dwChunkId)[0]
		<< ((char *)&avi_super_index.dwChunkId)[1]
		<< ((char *)&avi_super_index.dwChunkId)[2]
		<< ((char *)&avi_super_index.dwChunkId)[3]
		<< '\'' << endl
		<< "    dwReserved[0]:  " << ( int ) avi_super_index.dwReserved[ 0 ] << endl
		<< "    dwReserved[1]:  " << ( int ) avi_super_index.dwReserved[ 1 ] << endl
		<< "    dwReserved[2]:  " << ( int ) avi_super_index.dwReserved[ 2 ] << endl;
		for ( i = 0; i < avi_super_index.nEntriesInUse; ++i )
		{
			cout << ' ' << setw( 4 ) << setfill( ' ' ) << i
			<< ": qwOffset    : 0x" << setw( 12 ) << setfill( '0' ) << hex << avi_super_index.aIndex[ i ].qwOffset << endl
			<< "       dwSize      : 0x" << setw( 8 ) << avi_super_index.aIndex[ i ].dwSize << endl
			<< "       dwDuration  : " << dec << avi_super_index.aIndex[ i ].dwDuration << endl;
		}
	}
	else if ( entry.type == make_fourcc( "strf" ) )
	{
		if ( lastStreamType == make_fourcc( "auds" ) )
		{
			WAVEFORMATEX waveformatex;
			fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
			fail_neg( read( fd, &waveformatex, sizeof( WAVEFORMATEX ) ) );
			cout << "    waveformatex.wFormatTag     : " << waveformatex.wFormatTag << endl;
			cout << "    waveformatex.nChannels      : " << waveformatex.nChannels << endl;
			cout << "    waveformatex.nSamplesPerSec : " << waveformatex.nSamplesPerSec << endl;
			cout << "    waveformatex.nAvgBytesPerSec: " << waveformatex.nAvgBytesPerSec << endl;
			cout << "    waveformatex.nBlockAlign    : " << waveformatex.nBlockAlign << endl;
			cout << "    waveformatex.wBitsPerSample : " << waveformatex.wBitsPerSample << endl;
			cout << "    waveformatex.cbSize         : " << waveformatex.cbSize << endl;
		}
		else if ( lastStreamType == make_fourcc( "vids" ) )
		{
			BITMAPINFOHEADER bitmapinfo;
			fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
			fail_neg( read( fd, &bitmapinfo, sizeof( BITMAPINFOHEADER ) ) );
			cout << "    bitmapinfo.biSize         : " << bitmapinfo.biSize << endl;
			cout << "    bitmapinfo.biWidth        : " << bitmapinfo.biWidth << endl;
			cout << "    bitmapinfo.biHeight       : " << bitmapinfo.biHeight << endl;
			cout << "    bitmapinfo.biPlanes       : " << bitmapinfo.biPlanes << endl;
			cout << "    bitmapinfo.biBitCount     : " << bitmapinfo.biBitCount << endl;
			cout << "    bitmapinfo.biCompression  : " << bitmapinfo.biCompression << endl;
			cout << "    bitmapinfo.biSizeImage    : " << bitmapinfo.biSizeImage << endl;
			cout << "    bitmapinfo.biXPelsPerMeter: " << bitmapinfo.biXPelsPerMeter << endl;
			cout << "    bitmapinfo.biYPelsPerMeter: " << bitmapinfo.biYPelsPerMeter << endl;
			cout << "    bitmapinfo.biClrUsed      : " << bitmapinfo.biClrUsed << endl;
			cout << "    bitmapinfo.biClrImportant : " << bitmapinfo.biClrImportant << endl;
		}
		else if ( lastStreamType == make_fourcc( "iavs" ) )
		{
			DVINFO dvinfo;
			fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
			fail_neg( read( fd, &dvinfo, sizeof( DVINFO ) ) );
			cout << "    dvinfo.dwDVAAuxSrc : 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVAAuxSrc << endl;
			cout << "    dvinfo.dwDVAAuxCtl : 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVAAuxCtl << endl;
			cout << "    dvinfo.dwDVAAuxSrc1: 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVAAuxSrc1 << endl;
			cout << "    dvinfo.dwDVAAuxCtl1: 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVAAuxCtl1 << endl;
			cout << "    dvinfo.dwDVVAuxSrc : 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVVAuxSrc << endl;
			cout << "    dvinfo.dwDVVAuxCtl : 0x" << setw( 8 ) << setfill( '0' ) << hex << dvinfo.dwDVVAuxCtl << endl;
		}
	}

	/* This is the Standard Index. It is an array of offsets and
	   sizes relative to some start offset. */

	else if ( ( entry.type == make_fourcc( "ix00" ) ) || ( entry.type == make_fourcc( "ix01" ) ) )
	{

		int i;
		AVIStdIndex avi_std_index;

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, &avi_std_index, sizeof( AVIStdIndex ) ) );

		cout << "    wLongsPerEntry: " << ( int ) avi_std_index.wLongsPerEntry
		<< endl
		<< "    bIndexSubType:  " << ( int ) avi_std_index.bIndexSubType << endl
		<< "    bIndexType:     " << ( int ) avi_std_index.bIndexType << endl
		<< "    nEntriesInUse:  " << ( int ) avi_std_index.nEntriesInUse << endl
		<< "    dwChunkId:      '"
		<< ((char *)&avi_std_index.dwChunkId)[0]
		<< ((char *)&avi_std_index.dwChunkId)[1]
		<< ((char *)&avi_std_index.dwChunkId)[2]
		<< ((char *)&avi_std_index.dwChunkId)[3]
		<< '\'' << endl
		<< "    qwBaseOffset:   0x" << setw( 12 ) << hex << avi_std_index.qwBaseOffset << endl
		<< "    dwReserved:     " << dec << ( int ) avi_std_index.dwReserved << endl;
		for ( i = 0; i < avi_std_index.nEntriesInUse; ++i )
		{
			cout << ' ' << setw( 4 ) << setfill( ' ' ) << i
			<< ": dwOffset    : 0x" << setw( 8 ) << setfill( '0' ) << hex << avi_std_index.aIndex[ i ].dwOffset
			<< " (0x" << setw( 12 ) << avi_std_index.qwBaseOffset + avi_std_index.aIndex[ i ].dwOffset << ')' << endl
			<< "       dwSize      : 0x" << setw( 8 ) << avi_std_index.aIndex[ i ].dwSize << dec << endl;
		}

	}
	else if ( entry.type == make_fourcc( "idx1" ) )
	{

		int i;
		int numEntries = entry.length / sizeof( int ) / 4;
		DWORD *idx1 = new DWORD[ numEntries * 4 ];
		// FOURCC movi_list = FindDirectoryEntry(make_fourcc("movi"));

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, idx1, entry.length ) );

		for ( i = 0; i < numEntries; ++i )
		{

			cout << ' ' << setw( 4 ) << setfill( ' ' ) << i << setfill( '0' ) << ": dwChunkId : '"
			<< ((char *)&idx1[ i * 4 + 0 ])[0]
			<< ((char *)&idx1[ i * 4 + 0 ])[1]
			<< ((char *)&idx1[ i * 4 + 0 ])[2]
			<< ((char *)&idx1[ i * 4 + 0 ])[3]
			<< '\'' << endl
			<< "       dwType    : 0x" << setw( 8 ) << hex << idx1[ i * 4 + 1 ] << endl
			<< "       dwOffset  : 0x" << setw( 8 ) << idx1[ i * 4 + 2 ] << endl
			// << " (0x" << setw(8) << idx1[i * 4 + 2] + GetDirectoryEntry(movi_list).offset << ')' << endl
			<< "       dwSize    : 0x" << setw( 8 ) << idx1[ i * 4 + 3 ] << dec << endl;
		}

		delete[] idx1;
	}
	else if ( entry.type == make_fourcc( "dmlh" ) )
	{
		int i;
		int numEntries = entry.length / sizeof( int );
		DWORD *dmlh = new DWORD[ numEntries ];

		fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
		fail_neg( read( fd, dmlh, entry.length ) );

		for ( i = 0; i < numEntries; ++i )
		{
			cout << ' ' << setw( 4 ) << setfill( ' ' ) << i << setfill( '0' ) << ": "
			<< " dwTotalFrames: 0x" << setw( 8 ) << hex << dmlh[ i ]
			<< " (" << dec << dmlh[ i ] << ")" << endl;
		}
		delete[] dmlh;
	}
}


/** If this is not a movi list, read its contents
 
*/

void AVIFile::ParseList( int parent )
{
	FOURCC type;
	FOURCC name;
	DWORD length;
	int list;
	off_t pos;
	off_t listEnd;

	/* Read in the chunk header (type and length). */
	fail_neg( read( fd, &type, sizeof( type ) ) );
	fail_neg( read( fd, &length, sizeof( length ) ) );
	if ( length & 1 )
		length++;

	/* The contents of the list starts here. Obtain its offset. The list
	   name (4 bytes) is already part of the contents). */

	pos = lseek( fd, 0, SEEK_CUR );
	fail_if( pos == ( off_t ) - 1 );
	fail_neg( read( fd, &name, sizeof( name ) ) );

	/* if we encounter a movi list, do not read it. It takes too much time
	   and we don't need it anyway. */

	if ( name != make_fourcc( "movi" ) )
	{
		//    if (1) {

		/* Add an entry for this list. */
		list = AddDirectoryEntry( type, name, sizeof( name ), parent );

		/* Read in any chunks contained in this list. This list is the
		   parent for all chunks it contains. */

		listEnd = pos + length;
		while ( pos < listEnd )
		{
			ParseChunk( list );
			pos = lseek( fd, 0, SEEK_CUR );
			fail_if( pos == ( off_t ) - 1 );
		}
	}
	else
	{
		/* Add an entry for this list. */

		movi_list = AddDirectoryEntry( type, name, length, parent );

		pos = lseek( fd, length - 4, SEEK_CUR );
		fail_if( pos == ( off_t ) - 1 );
	}
}


void AVIFile::ParseRIFF()
{
	RIFFFile::ParseRIFF();

	avih_chunk = FindDirectoryEntry( make_fourcc( "avih" ) );
	if ( avih_chunk != -1 )
		ReadChunk( avih_chunk, ( void* ) & mainHdr, sizeof( MainAVIHeader ) );
}


void AVIFile::ReadIndex()
{
	indx_chunk[ 0 ] = FindDirectoryEntry( make_fourcc( "indx" ) );
	if ( indx_chunk[ 0 ] != -1 )
	{
		ReadChunk( indx_chunk[ 0 ], ( void* ) indx[ 0 ], sizeof( AVISuperIndex ) );
		index_type = AVI_LARGE_INDEX;

		/* recalc number of frames from each index */
		mainHdr.dwTotalFrames = 0;
		for ( int i = 0;
		        i < indx[ 0 ] ->nEntriesInUse;
		        mainHdr.dwTotalFrames += indx[ 0 ] ->aIndex[ i++ ].dwDuration )
			;
		return ;
	}
	idx1_chunk = FindDirectoryEntry( make_fourcc( "idx1" ) );
	if ( idx1_chunk != -1 )
	{
		ReadChunk( idx1_chunk, ( void* ) idx1, sizeof( AVISuperIndex ) );
		idx1->nEntriesInUse = GetDirectoryEntry( idx1_chunk ).length / 16;
		index_type = AVI_SMALL_INDEX;

		/* recalc number of frames from the simple index */
		int frameNumIndex = 0;
		FOURCC chunkID1 = make_fourcc( "00dc" );
		FOURCC chunkID2 = make_fourcc( "00db" );
		for ( int i = 0; i < idx1->nEntriesInUse; ++i )
		{
			if ( idx1->aIndex[ i ].dwChunkId == chunkID1 ||
			        idx1->aIndex[ i ].dwChunkId == chunkID2 )
			{
				++frameNumIndex;
			}
		}
		mainHdr.dwTotalFrames = frameNumIndex;
		return ;
	}
}


void AVIFile::FlushIndx( int stream )
{
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;
	int i;

	/* Write out the previous index. When this function is
	   entered for the first time, there is no index to
	   write.  Note: this may be an expensive operation
	   because of a time consuming seek to the former file
	   position. */

	if ( ix_chunk[ stream ] != -1 )
		WriteChunk( ix_chunk[ stream ], ix[ stream ] );

	/* make a new ix chunk. */

	if ( stream == 0 )
		type = make_fourcc( "ix00" );
	else
		type = make_fourcc( "ix01" );
	ix_chunk[ stream ] = AddDirectoryEntry( type, 0, sizeof( AVIStdIndex ), movi_list );
	GetDirectoryEntry( ix_chunk[ stream ], type, name, length, offset, parent );

	/* fill out all required fields. The offsets in the
	   array are relative to qwBaseOffset, so fill in the
	   offset to the next free location in the file
	   there. */

	ix[ stream ] ->wLongsPerEntry = 2;
	ix[ stream ] ->bIndexSubType = 0;
	ix[ stream ] ->bIndexType = KINO_AVI_INDEX_OF_CHUNKS;
	ix[ stream ] ->nEntriesInUse = 0;
	ix[ stream ] ->dwChunkId = indx[ stream ] ->dwChunkId;
	ix[ stream ] ->qwBaseOffset = offset + length;
	ix[ stream ] ->dwReserved = 0;

	for ( i = 0; i < IX00_INDEX_SIZE; ++i )
	{
		ix[ stream ] ->aIndex[ i ].dwOffset = 0;
		ix[ stream ] ->aIndex[ i ].dwSize = 0;
	}

	/* add a reference to this new index in our super
	   index. */

	i = indx[ stream ] ->nEntriesInUse++;
	indx[ stream ] ->aIndex[ i ].qwOffset = offset - RIFF_HEADERSIZE;
	indx[ stream ] ->aIndex[ i ].dwSize = length + RIFF_HEADERSIZE;
	indx[ stream ] ->aIndex[ i ].dwDuration = 0;
}


void AVIFile::UpdateIndx( int stream, int chunk, int duration )
{
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;
	int i;

	/* update the appropiate entry in the super index. It reflects
	   the number of frames in the referenced index. */

	i = indx[ stream ] ->nEntriesInUse - 1;
	indx[ stream ] ->aIndex[ i ].dwDuration += duration;

	/* update the standard index. Calculate the file position of
	   the new frame. */

	GetDirectoryEntry( chunk, type, name, length, offset, parent );

	indx[ stream ] ->dwChunkId = type;
	i = ix[ stream ] ->nEntriesInUse++;
	ix[ stream ] ->aIndex[ i ].dwOffset = offset - ix[ stream ] ->qwBaseOffset;
	ix[ stream ] ->aIndex[ i ].dwSize = length;
}


void AVIFile::UpdateIdx1( int chunk, int flags )
{
	if ( idx1->nEntriesInUse < 20000 )
	{
		FOURCC type;
		FOURCC name;
		off_t length;
		off_t offset;
		int parent;

		GetDirectoryEntry( chunk, type, name, length, offset, parent );

		idx1->aIndex[ idx1->nEntriesInUse ].dwChunkId = type;
		idx1->aIndex[ idx1->nEntriesInUse ].dwFlags = flags;
		idx1->aIndex[ idx1->nEntriesInUse ].dwOffset = offset - GetDirectoryEntry( movi_list ).offset - RIFF_HEADERSIZE;
		idx1->aIndex[ idx1->nEntriesInUse ].dwSize = length;
		idx1->nEntriesInUse++;
	}
}

bool AVIFile::verifyStreamFormat( FOURCC type )
{
	int i, j = 0;
	AVIStreamHeader	avi_stream_header;
	BITMAPINFOHEADER bih;
	FOURCC strh = make_fourcc( "strh" );
	FOURCC strf = make_fourcc( "strf" );

	while ( ( i = FindDirectoryEntry( strh, j++ ) ) != -1 )
	{
		ReadChunk( i, ( void* ) & avi_stream_header, sizeof( AVIStreamHeader ) );
		if ( avi_stream_header.fccHandler == type )
			return true;
	}
	j = 0;
	while ( ( i = FindDirectoryEntry( strf, j++ ) ) != -1 )
	{
		ReadChunk( i, ( void* ) & bih, sizeof( BITMAPINFOHEADER ) );
		if ( ( FOURCC ) bih.biCompression == type )
			return true;
	}

	return false;
}

bool AVIFile::verifyStream( FOURCC type )
{
	int i, j = 0;
	AVIStreamHeader	avi_stream_header;
	FOURCC strh = make_fourcc( "strh" );

	while ( ( i = FindDirectoryEntry( strh, j++ ) ) != -1 )
	{
		ReadChunk( i, ( void* ) & avi_stream_header, sizeof( AVIStreamHeader ) );
		if ( avi_stream_header.fccType == type )
			return true;
	}
	return false;
}

bool AVIFile::isOpenDML( void )
{
	int i, j = 0;
	FOURCC dmlh = make_fourcc( "dmlh" );

	while ( ( i = FindDirectoryEntry( dmlh, j++ ) ) != -1 )
	{
		return true;
	}
	return false;
}

AVI1File::AVI1File() : AVIFile()
{
	memset( &dvinfo, 0, sizeof( dvinfo ) );
}


AVI1File::~AVI1File()
{}


/* Initialize the AVI structure to its initial state, either for PAL
   or NTSC format */

void AVI1File::Init( int format, int sampleFrequency, int indexType )
{
	int num_blocks;
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;

	assert( ( format == AVI_PAL ) || ( format == AVI_NTSC ) );

	AVIFile::Init( format, sampleFrequency, indexType );

	switch ( format )
	{
	case AVI_PAL:
		mainHdr.dwWidth = 720;
		mainHdr.dwHeight = 576;

		streamHdr[ 0 ].dwScale = 1;
		streamHdr[ 0 ].dwRate = 25;
		streamHdr[ 0 ].dwSuggestedBufferSize = 144008;

		/* initialize the 'strf' chunk */

		/* Meaning of the DV stream format chunk per Microsoft
		   dwDVAAuxSrc
		      Specifies the Audio Auxiliary Data Source Pack for the first audio block
		      (first 5 DV DIF sequences for 525-60 systems or 6 DV DIF sequences for 625-50 systems) of
		      a frame. A DIF sequence is a data block that contains 150 DIF blocks. A DIF block consists
		      of 80 bytes. The Audio Auxiliary Data Source Pack is defined in section D.7.1 of Part 2,
		      Annex D, "The Pack Header Table and Contents of Packs" of the Specification of
		      Consumer-use Digital VCRs.
		   dwDVAAuxCtl
		      Specifies the Audio Auxiliary Data Source Control Pack for the first audio block of a
		      frame. The Audio Auxiliary Data Control Pack is defined in section D.7.2 of Part 2,
		      Annex D, "The Pack Header Table and Contents of Packs" of the Specification of
		      Consumer-use Digital VCRs.
		   dwDVAAuxSrc1
		      Specifies the Audio Auxiliary Data Source Pack for the second audio block
		      (second 5 DV DIF sequences for 525-60 systems or 6 DV DIF sequences for 625-50 systems) of a frame.
		   dwDVAAuxCtl1
		      Specifies the Audio Auxiliary Data Source Control Pack for the second audio block of a frame.
		   dwDVVAuxSrc
		      Specifies the Video Auxiliary Data Source Pack as defined in section D.8.1 of Part 2,
		      Annex D, "The Pack Header Table and Contents of Packs" of the Specification of
		      Consumer-use Digital VCRs.
		   dwDVVAuxCtl
		      Specifies the Video Auxiliary Data Source Control Pack as defined in section D.8.2 of Part 2,
		      Annex D, "The Pack Header Table and Contents of Packs" of the Specification of
		      Consumer-use Digital VCRs.
		   dwDVReserved[2]
		      Reserved. Set this array to zero.   
		*/

		dvinfo.dwDVAAuxSrc = 0xd1e030d0;
		dvinfo.dwDVAAuxCtl = 0xffa0cf3f;
		dvinfo.dwDVAAuxSrc1 = 0xd1e03fd0;
		dvinfo.dwDVAAuxCtl1 = 0xffa0cf3f;
		dvinfo.dwDVVAuxSrc = 0xff20ffff;
		dvinfo.dwDVVAuxCtl = 0xfffdc83f;
		dvinfo.dwDVReserved[ 0 ] = 0;
		dvinfo.dwDVReserved[ 1 ] = 0;
		break;

	case AVI_NTSC:
		mainHdr.dwWidth = 720;
		mainHdr.dwHeight = 480;

		streamHdr[ 0 ].dwScale = 1001;
		streamHdr[ 0 ].dwRate = 30000;
		streamHdr[ 0 ].dwSuggestedBufferSize = 120008;

		/* initialize the 'strf' chunk */
		dvinfo.dwDVAAuxSrc = 0xc0c000c0;
		dvinfo.dwDVAAuxCtl = 0xffa0cf3f;
		dvinfo.dwDVAAuxSrc1 = 0xc0c001c0;
		dvinfo.dwDVAAuxCtl1 = 0xffa0cf3f;
		dvinfo.dwDVVAuxSrc = 0xff80ffff;
		dvinfo.dwDVVAuxCtl = 0xfffcc83f;
		dvinfo.dwDVReserved[ 0 ] = 0;
		dvinfo.dwDVReserved[ 1 ] = 0;
		break;

	default:   /* no default allowed */
		assert( 0 );
		break;
	}

	indx[ 0 ] ->dwChunkId = make_fourcc( "00__" );

	/* Initialize the 'strh' chunk */

	streamHdr[ 0 ].fccType = make_fourcc( "iavs" );
	streamHdr[ 0 ].fccHandler = make_fourcc( "dvsd" );
	streamHdr[ 0 ].dwFlags = 0;
	streamHdr[ 0 ].wPriority = 0;
	streamHdr[ 0 ].wLanguage = 0;
	streamHdr[ 0 ].dwInitialFrames = 0;
	streamHdr[ 0 ].dwStart = 0;
	streamHdr[ 0 ].dwLength = 0;
	streamHdr[ 0 ].dwQuality = 0;
	streamHdr[ 0 ].dwSampleSize = 0;
	streamHdr[ 0 ].rcFrame.top = 0;
	streamHdr[ 0 ].rcFrame.bottom = 0;
	streamHdr[ 0 ].rcFrame.left = 0;
	streamHdr[ 0 ].rcFrame.right = 0;

	/* This is a simple directory structure setup. For details see the
	   "OpenDML AVI File Format Extensions" document.
	   
	   An AVI file contains basically two types of objects, a
	   "chunk" and a "list" object. The list object contains any
	   number of chunks. Since a list is also a chunk, it is
	   possible to create a hierarchical "list of lists"
	   structure.

	   Every AVI file starts with a "RIFF" object, which is a list
	   of several other required objects. The actual DV data is
	   contained in a "movi" list, each frame is in its own chunk.

	   Old AVI files (pre OpenDML V. 1.02) contain only one RIFF
	   chunk of less than 1 GByte size per file. The current
	   format which allow for almost arbitrary sizes can contain
	   several RIFF chunks of less than 1 GByte size. Old software
	   however would only deal with the first RIFF chunk.

	   Note that the first entry (FILE) isn�t actually part
	   of the AVI file. I use this (pseudo-) directory entry to
	   keep track of the RIFF chunks and their positions in the
	   AVI file.
	*/

	/* Create the container directory entry */

	file_list = AddDirectoryEntry( make_fourcc( "FILE" ), make_fourcc( "FILE" ), 0, RIFF_NO_PARENT );

	/* Create a basic directory structure. Only chunks defined from here on will be written to the AVI file. */

	riff_list = AddDirectoryEntry( make_fourcc( "RIFF" ), make_fourcc( "AVI " ), RIFF_LISTSIZE, file_list );
	hdrl_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "hdrl" ), RIFF_LISTSIZE, riff_list );
	avih_chunk = AddDirectoryEntry( make_fourcc( "avih" ), 0, sizeof( MainAVIHeader ), hdrl_list );
	strl_list[ 0 ] = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "strl" ), RIFF_LISTSIZE, hdrl_list );
	strh_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "strh" ), 0, sizeof( AVIStreamHeader ), strl_list[ 0 ] );
	strf_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "strf" ), 0, sizeof( dvinfo ), strl_list[ 0 ] );
	if ( index_type & AVI_LARGE_INDEX )
		indx_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "indx" ), 0, sizeof( AVISuperIndex ), strl_list[ 0 ] );

	odml_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "odml" ), RIFF_LISTSIZE, hdrl_list );
	dmlh_chunk = AddDirectoryEntry( make_fourcc( "dmlh" ), 0, 0x00f8, odml_list );

	/* align movi list to block */
	GetDirectoryEntry( hdrl_list, type, name, length, offset, parent );
	num_blocks = length / PADDING_SIZE + 1;
	length = num_blocks * PADDING_SIZE - length - 5 * RIFF_HEADERSIZE; // why 5?
	junk_chunk = AddDirectoryEntry( make_fourcc( "JUNK" ), 0, length, riff_list );

	movi_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "movi" ), RIFF_LISTSIZE, riff_list );

	/* The ix00 chunk will be added dynamically to the movi_list in avi_write_frame
	          as needed */

	ix_chunk[ 0 ] = -1;
}


/* Write a DV video frame. This is somewhat complex... */

#if 0
bool AVI1File::WriteFrame( const Frame &frame )
{
	int frame_chunk;
	int junk_chunk;
	int num_blocks;
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;

	/* exit if no large index and 1GB reached */
	if ( !( index_type & AVI_LARGE_INDEX ) && isUpdateIdx1 == false )
		return false;

	/* Check if we need a new ix00 Standard Index. It has a
	   capacity of IX00_INDEX_SIZE frames. Whenever we exceed that
	   number, we need a new index. The new ix00 chunk is also
	   part of the movi list. */

	if ( ( index_type & AVI_LARGE_INDEX ) && ( ( ( streamHdr[ 0 ].dwLength - 0 ) % IX00_INDEX_SIZE ) == 0 ) )
		FlushIndx( 0 );

	/* Write the DV frame data.

	   Make a new 00__ chunk for the new frame, write out the
	   frame. */

	frame_chunk = AddDirectoryEntry( make_fourcc( "00__" ), 0, frame.GetFrameSize(), movi_list );
	if ( ( index_type & AVI_LARGE_INDEX ) && ( streamHdr[ 0 ].dwLength % IX00_INDEX_SIZE ) == 0 )
	{
		GetDirectoryEntry( frame_chunk, type, name, length, offset, parent );
		ix[ 0 ] ->qwBaseOffset = offset - RIFF_HEADERSIZE;
	}
	WriteChunk( frame_chunk, frame.data );
	//    num_blocks = (frame.GetFrameSize() + RIFF_HEADERSIZE) / PADDING_SIZE + 1;
	//    length = num_blocks * PADDING_SIZE - frame.GetFrameSize() - 2 * RIFF_HEADERSIZE;
	//    junk_chunk = AddDirectoryEntry(make_fourcc("JUNK"), 0, length, movi_list);
	//    WriteChunk(junk_chunk, g_zeroes);

	if ( index_type & AVI_LARGE_INDEX )
		UpdateIndx( 0, frame_chunk, 1 );
	if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
		UpdateIdx1( frame_chunk, 0x10 );

	/* update some variables with the new frame count. */

	if ( isUpdateIdx1 )
		++mainHdr.dwTotalFrames;
	++streamHdr[ 0 ].dwLength;
	++dmlh[ 0 ];

	/* Find out if the current riff list is close to 1 GByte in
	   size. If so, start a new (extended) RIFF. The only allowed
	   item in the new RIFF chunk is a movi list (with video
	   frames and indexes as usual). */

	GetDirectoryEntry( riff_list, type, name, length, offset, parent );
	if ( length > 0x3f000000 )
	{
		/* write idx1 only once and before end of first GB */
		if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
		{
			int idx1_chunk = AddDirectoryEntry( make_fourcc( "idx1" ), 0, idx1->nEntriesInUse * 16, riff_list );
			WriteChunk( idx1_chunk, ( void* ) idx1 );
		}
		isUpdateIdx1 = false;

		if ( index_type & AVI_LARGE_INDEX )
		{
			/* pad out to 1GB */
			//GetDirectoryEntry(riff_list, type, name, length, offset, parent);
			//junk_chunk = AddDirectoryEntry(make_fourcc("JUNK"), 0, PADDING_1GB - length - 5 * RIFF_HEADERSIZE, riff_list);
			//WriteChunk(junk_chunk, g_zeroes);

			/* padding for alignment */
			GetDirectoryEntry( riff_list, type, name, length, offset, parent );
			num_blocks = ( length + 4 * RIFF_HEADERSIZE ) / PADDING_SIZE + 1;
			length = ( num_blocks * PADDING_SIZE ) - length - 4 * RIFF_HEADERSIZE - 2 * RIFF_LISTSIZE;
			if ( length > 0 )
			{
				junk_chunk = AddDirectoryEntry( make_fourcc( "JUNK" ), 0, length, riff_list );
				WriteChunk( junk_chunk, g_zeroes );
			}

			riff_list = AddDirectoryEntry( make_fourcc( "RIFF" ), make_fourcc( "AVIX" ), RIFF_LISTSIZE, file_list );
			movi_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "movi" ), RIFF_LISTSIZE, riff_list );
		}
	}
	return true;
}
#endif

void AVI1File::WriteRIFF()
{

	WriteChunk( avih_chunk, ( void* ) & mainHdr );
	WriteChunk( strh_chunk[ 0 ], ( void* ) & streamHdr[ 0 ] );
	WriteChunk( strf_chunk[ 0 ], ( void* ) & dvinfo );
	WriteChunk( dmlh_chunk, ( void* ) & dmlh );

	if ( index_type & AVI_LARGE_INDEX )
	{
		WriteChunk( indx_chunk[ 0 ], ( void* ) indx[ 0 ] );
		WriteChunk( ix_chunk[ 0 ], ( void* ) ix[ 0 ] );
	}

	if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
	{
		int idx1_chunk = AddDirectoryEntry( make_fourcc( "idx1" ), 0, idx1->nEntriesInUse * 16, riff_list );
		WriteChunk( idx1_chunk, ( void* ) idx1 );
	}

	RIFFFile::WriteRIFF();
}


AVI2File::AVI2File() : AVIFile()
{}


AVI2File::~AVI2File()
{}


/* Initialize the AVI structure to its initial state, either for PAL
   or NTSC format */

void AVI2File::Init( int format, int sampleFrequency, int indexType )
{
	int num_blocks;
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;

	assert( ( format == AVI_PAL ) || ( format == AVI_NTSC ) );

	AVIFile::Init( format, sampleFrequency, indexType );

	switch ( format )
	{

	case AVI_PAL:
		mainHdr.dwStreams = 2;
		mainHdr.dwWidth = 720;
		mainHdr.dwHeight = 576;

		/* Initialize the 'strh' chunk */

		streamHdr[ 0 ].fccType = make_fourcc( "vids" );
		streamHdr[ 0 ].fccHandler = make_fourcc( "dvsd" );
		streamHdr[ 0 ].dwFlags = 0;
		streamHdr[ 0 ].wPriority = 0;
		streamHdr[ 0 ].wLanguage = 0;
		streamHdr[ 0 ].dwInitialFrames = 0;
		streamHdr[ 0 ].dwScale = 1;
		streamHdr[ 0 ].dwRate = 25;
		streamHdr[ 0 ].dwStart = 0;
		streamHdr[ 0 ].dwLength = 0;
		streamHdr[ 0 ].dwSuggestedBufferSize = 144008;
		streamHdr[ 0 ].dwQuality = -1;
		streamHdr[ 0 ].dwSampleSize = 0;
		streamHdr[ 0 ].rcFrame.top = 0;
		streamHdr[ 0 ].rcFrame.bottom = 0;
		streamHdr[ 0 ].rcFrame.left = 0;
		streamHdr[ 0 ].rcFrame.right = 0;

		bitmapinfo.biSize = sizeof( bitmapinfo );
		bitmapinfo.biWidth = 720;
		bitmapinfo.biHeight = 576;
		bitmapinfo.biPlanes = 1;
		bitmapinfo.biBitCount = 24;
		bitmapinfo.biCompression = make_fourcc( "dvsd" );
		bitmapinfo.biSizeImage = 144000;
		bitmapinfo.biXPelsPerMeter = 0;
		bitmapinfo.biYPelsPerMeter = 0;
		bitmapinfo.biClrUsed = 0;
		bitmapinfo.biClrImportant = 0;

		streamHdr[ 1 ].fccType = make_fourcc( "auds" );
		streamHdr[ 1 ].fccHandler = 0;
		streamHdr[ 1 ].dwFlags = 0;
		streamHdr[ 1 ].wPriority = 0;
		streamHdr[ 1 ].wLanguage = 0;
		streamHdr[ 1 ].dwInitialFrames = 0;
		streamHdr[ 1 ].dwScale = 2 * 2;
		streamHdr[ 1 ].dwRate = sampleFrequency * 2 * 2;
		streamHdr[ 1 ].dwStart = 0;
		streamHdr[ 1 ].dwLength = 0;
		streamHdr[ 1 ].dwSuggestedBufferSize = 8192;
		streamHdr[ 1 ].dwQuality = -1;
		streamHdr[ 1 ].dwSampleSize = 2 * 2;
		streamHdr[ 1 ].rcFrame.top = 0;
		streamHdr[ 1 ].rcFrame.bottom = 0;
		streamHdr[ 1 ].rcFrame.left = 0;
		streamHdr[ 1 ].rcFrame.right = 0;

		break;

	case AVI_NTSC:
		mainHdr.dwTotalFrames = 0;
		mainHdr.dwStreams = 2;
		mainHdr.dwWidth = 720;
		mainHdr.dwHeight = 480;

		/* Initialize the 'strh' chunk */

		streamHdr[ 0 ].fccType = make_fourcc( "vids" );
		streamHdr[ 0 ].fccHandler = make_fourcc( "dvsd" );
		streamHdr[ 0 ].dwFlags = 0;
		streamHdr[ 0 ].wPriority = 0;
		streamHdr[ 0 ].wLanguage = 0;
		streamHdr[ 0 ].dwInitialFrames = 0;
		streamHdr[ 0 ].dwScale = 1001;
		streamHdr[ 0 ].dwRate = 30000;
		streamHdr[ 0 ].dwStart = 0;
		streamHdr[ 0 ].dwLength = 0;
		streamHdr[ 0 ].dwSuggestedBufferSize = 120008;
		streamHdr[ 0 ].dwQuality = -1;
		streamHdr[ 0 ].dwSampleSize = 0;
		streamHdr[ 0 ].rcFrame.top = 0;
		streamHdr[ 0 ].rcFrame.bottom = 0;
		streamHdr[ 0 ].rcFrame.left = 0;
		streamHdr[ 0 ].rcFrame.right = 0;

		bitmapinfo.biSize = sizeof( bitmapinfo );
		bitmapinfo.biWidth = 720;
		bitmapinfo.biHeight = 480;
		bitmapinfo.biPlanes = 1;
		bitmapinfo.biBitCount = 24;
		bitmapinfo.biCompression = make_fourcc( "dvsd" );
		bitmapinfo.biSizeImage = 120000;
		bitmapinfo.biXPelsPerMeter = 0;
		bitmapinfo.biYPelsPerMeter = 0;
		bitmapinfo.biClrUsed = 0;
		bitmapinfo.biClrImportant = 0;

		streamHdr[ 1 ].fccType = make_fourcc( "auds" );
		streamHdr[ 1 ].fccHandler = 0;
		streamHdr[ 1 ].dwFlags = 0;
		streamHdr[ 1 ].wPriority = 0;
		streamHdr[ 1 ].wLanguage = 0;
		streamHdr[ 1 ].dwInitialFrames = 1;
		streamHdr[ 1 ].dwScale = 2 * 2;
		streamHdr[ 1 ].dwRate = sampleFrequency * 2 * 2;
		streamHdr[ 1 ].dwStart = 0;
		streamHdr[ 1 ].dwLength = 0;
		streamHdr[ 1 ].dwSuggestedBufferSize = 8192;
		streamHdr[ 1 ].dwQuality = 0;
		streamHdr[ 1 ].dwSampleSize = 2 * 2;
		streamHdr[ 1 ].rcFrame.top = 0;
		streamHdr[ 1 ].rcFrame.bottom = 0;
		streamHdr[ 1 ].rcFrame.left = 0;
		streamHdr[ 1 ].rcFrame.right = 0;

		break;
	}
	waveformatex.wFormatTag = 1;
	waveformatex.nChannels = 2;
	waveformatex.nSamplesPerSec = sampleFrequency;
	waveformatex.nAvgBytesPerSec = sampleFrequency * 2 * 2;
	waveformatex.nBlockAlign = 4;
	waveformatex.wBitsPerSample = 16;
	waveformatex.cbSize = 0;

	file_list = AddDirectoryEntry( make_fourcc( "FILE" ), make_fourcc( "FILE" ), 0, RIFF_NO_PARENT );

	/* Create a basic directory structure. Only chunks defined from here on will be written to the AVI file. */

	riff_list = AddDirectoryEntry( make_fourcc( "RIFF" ), make_fourcc( "AVI " ), RIFF_LISTSIZE, file_list );
	hdrl_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "hdrl" ), RIFF_LISTSIZE, riff_list );
	avih_chunk = AddDirectoryEntry( make_fourcc( "avih" ), 0, sizeof( MainAVIHeader ), hdrl_list );

	strl_list[ 0 ] = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "strl" ), RIFF_LISTSIZE, hdrl_list );
	strh_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "strh" ), 0, sizeof( AVIStreamHeader ), strl_list[ 0 ] );
	strf_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "strf" ), 0, sizeof( BITMAPINFOHEADER ), strl_list[ 0 ] );
	if ( index_type & AVI_LARGE_INDEX )
	{
		indx_chunk[ 0 ] = AddDirectoryEntry( make_fourcc( "indx" ), 0, sizeof( AVISuperIndex ), strl_list[ 0 ] );
		ix_chunk[ 0 ] = -1;
		indx[ 0 ] ->dwChunkId = make_fourcc( "00dc" );
	}

	strl_list[ 1 ] = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "strl" ), RIFF_LISTSIZE, hdrl_list );
	strh_chunk[ 1 ] = AddDirectoryEntry( make_fourcc( "strh" ), 0, sizeof( AVIStreamHeader ), strl_list[ 1 ] );
	strf_chunk[ 1 ] = AddDirectoryEntry( make_fourcc( "strf" ), 0, sizeof( WAVEFORMATEX ) - 2, strl_list[ 1 ] );
	junk_chunk = AddDirectoryEntry( make_fourcc( "JUNK" ), 0, 2, strl_list[ 1 ] );
	if ( index_type & AVI_LARGE_INDEX )
	{
		indx_chunk[ 1 ] = AddDirectoryEntry( make_fourcc( "indx" ), 0, sizeof( AVISuperIndex ), strl_list[ 1 ] );
		ix_chunk[ 1 ] = -1;
		indx[ 1 ] ->dwChunkId = make_fourcc( "01wb" );

		odml_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "odml" ), RIFF_LISTSIZE, hdrl_list );
		dmlh_chunk = AddDirectoryEntry( make_fourcc( "dmlh" ), 0, 0x00f8, odml_list );
	}

	/* align movi list to block */
	GetDirectoryEntry( hdrl_list, type, name, length, offset, parent );
	num_blocks = length / PADDING_SIZE + 1;
	length = num_blocks * PADDING_SIZE - length - 5 * RIFF_HEADERSIZE; // why 5 headers?
	junk_chunk = AddDirectoryEntry( make_fourcc( "JUNK" ), 0, length, riff_list );

	movi_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "movi" ), RIFF_LISTSIZE, riff_list );

	idx1->aIndex[ idx1->nEntriesInUse ].dwChunkId = make_fourcc( "7Fxx" );
	idx1->aIndex[ idx1->nEntriesInUse ].dwFlags = 0;
	idx1->aIndex[ idx1->nEntriesInUse ].dwOffset = 0;
	idx1->aIndex[ idx1->nEntriesInUse ].dwSize = 0;
	idx1->nEntriesInUse++;
}


void AVI2File::WriteRIFF()
{
	WriteChunk( avih_chunk, ( void* ) & mainHdr );
	WriteChunk( strh_chunk[ 0 ], ( void* ) & streamHdr[ 0 ] );
	WriteChunk( strf_chunk[ 0 ], ( void* ) & bitmapinfo );
	if ( index_type & AVI_LARGE_INDEX )
	{
		WriteChunk( dmlh_chunk, ( void* ) & dmlh );
		WriteChunk( indx_chunk[ 0 ], ( void* ) indx[ 0 ] );
		WriteChunk( ix_chunk[ 0 ], ( void* ) ix[ 0 ] );
	}
	WriteChunk( strh_chunk[ 1 ], ( void* ) & streamHdr[ 1 ] );
	WriteChunk( strf_chunk[ 1 ], ( void* ) & waveformatex );
	if ( index_type & AVI_LARGE_INDEX )
	{
		WriteChunk( indx_chunk[ 1 ], ( void* ) indx[ 1 ] );
		WriteChunk( ix_chunk[ 1 ], ( void* ) ix[ 1 ] );
	}

	if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
	{
		int idx1_chunk = AddDirectoryEntry( make_fourcc( "idx1" ), 0, idx1->nEntriesInUse * 16, riff_list );
		WriteChunk( idx1_chunk, ( void* ) idx1 );
	}
	RIFFFile::WriteRIFF();
}


/** Write a DV video frame
 
    \param frame the frame to write
*/

#if 0
bool AVI2File::WriteFrame( const Frame &frame )
{
	int audio_chunk;
	int frame_chunk;
	int junk_chunk;
	char soundbuf[ 20000 ];
	int	audio_size;
	int num_blocks;
	FOURCC type;
	FOURCC name;
	off_t length;
	off_t offset;
	int parent;

	/* exit if no large index and 1GB reached */
	if ( !( index_type & AVI_LARGE_INDEX ) && isUpdateIdx1 == false )
		return false;

	/* Check if we need a new ix00 Standard Index. It has a
	   capacity of IX00_INDEX_SIZE frames. Whenever we exceed that
	   number, we need a new index. The new ix00 chunk is also
	   part of the movi list. */

	if ( ( index_type & AVI_LARGE_INDEX ) && ( ( ( streamHdr[ 0 ].dwLength - 0 ) % IX00_INDEX_SIZE ) == 0 ) )
	{
		FlushIndx( 0 );
		FlushIndx( 1 );
	}

	/* Write audio data if we have it */

	audio_size = frame.ExtractAudio( soundbuf );
	if ( audio_size > 0 )
	{
		audio_chunk = AddDirectoryEntry( make_fourcc( "01wb" ), 0, audio_size, movi_list );
		if ( ( index_type & AVI_LARGE_INDEX ) && ( streamHdr[ 0 ].dwLength % IX00_INDEX_SIZE ) == 0 )
		{
			GetDirectoryEntry( audio_chunk, type, name, length, offset, parent );
			ix[ 1 ] ->qwBaseOffset = offset - RIFF_HEADERSIZE;
		}
		WriteChunk( audio_chunk, soundbuf );
		//        num_blocks = (audio_size + RIFF_HEADERSIZE) / PADDING_SIZE + 1;
		//        length = num_blocks * PADDING_SIZE - audio_size - 2 * RIFF_HEADERSIZE;
		//        junk_chunk = AddDirectoryEntry(make_fourcc("JUNK"), 0, length, movi_list);
		//        WriteChunk(junk_chunk, g_zeroes);
		if ( index_type & AVI_LARGE_INDEX )
			UpdateIndx( 1, audio_chunk, audio_size / waveformatex.nChannels / 2 );
		if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
			UpdateIdx1( audio_chunk, 0x00 );
		streamHdr[ 1 ].dwLength += audio_size / waveformatex.nChannels / 2;

	}

	/* Write video data */

	frame_chunk = AddDirectoryEntry( make_fourcc( "00dc" ), 0, frame.GetFrameSize(), movi_list );
	if ( ( index_type & AVI_LARGE_INDEX ) && ( streamHdr[ 0 ].dwLength % IX00_INDEX_SIZE ) == 0 )
	{
		GetDirectoryEntry( frame_chunk, type, name, length, offset, parent );
		ix[ 0 ] ->qwBaseOffset = offset - RIFF_HEADERSIZE;
	}
	WriteChunk( frame_chunk, frame.data );
	//    num_blocks = (frame.GetFrameSize() + RIFF_HEADERSIZE) / PADDING_SIZE + 1;
	//    length = num_blocks * PADDING_SIZE - frame.GetFrameSize() - 2 * RIFF_HEADERSIZE;
	//    junk_chunk = AddDirectoryEntry(make_fourcc("JUNK"), 0, length, movi_list);
	//    WriteChunk(junk_chunk, g_zeroes);
	if ( index_type & AVI_LARGE_INDEX )
		UpdateIndx( 0, frame_chunk, 1 );
	if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
		UpdateIdx1( frame_chunk, 0x10 );

	/* update some variables with the new frame count. */

	if ( isUpdateIdx1 )
		++mainHdr.dwTotalFrames;
	++streamHdr[ 0 ].dwLength;
	++dmlh[ 0 ];

	/* Find out if the current riff list is close to 1 GByte in
	   size. If so, start a new (extended) RIFF. The only allowed
	   item in the new RIFF chunk is a movi list (with video
	   frames and indexes as usual). */

	GetDirectoryEntry( riff_list, type, name, length, offset, parent );
	if ( length > 0x3f000000 )
	{

		/* write idx1 only once and before end of first GB */
		if ( ( index_type & AVI_SMALL_INDEX ) && isUpdateIdx1 )
		{
			int idx1_chunk = AddDirectoryEntry( make_fourcc( "idx1" ), 0, idx1->nEntriesInUse * 16, riff_list );
			WriteChunk( idx1_chunk, ( void* ) idx1 );
		}
		isUpdateIdx1 = false;

		if ( index_type & AVI_LARGE_INDEX )
		{
			/* padding for alignment */
			GetDirectoryEntry( riff_list, type, name, length, offset, parent );
			num_blocks = ( length + 4 * RIFF_HEADERSIZE ) / PADDING_SIZE + 1;
			length = ( num_blocks * PADDING_SIZE ) - length - 4 * RIFF_HEADERSIZE - 2 * RIFF_LISTSIZE;
			if ( length > 0 )
			{
				junk_chunk = AddDirectoryEntry( make_fourcc( "JUNK" ), 0, length, riff_list );
				WriteChunk( junk_chunk, g_zeroes );
			}

			riff_list = AddDirectoryEntry( make_fourcc( "RIFF" ), make_fourcc( "AVIX" ), RIFF_LISTSIZE, file_list );
			movi_list = AddDirectoryEntry( make_fourcc( "LIST" ), make_fourcc( "movi" ), RIFF_LISTSIZE, riff_list );
		}
	}
	return true;
}
#endif

void AVI1File::setDVINFO( DVINFO &info )
{
	// do not do this until debugged audio against DirectShow
	return ;

	dvinfo.dwDVAAuxSrc = info.dwDVAAuxSrc;
	dvinfo.dwDVAAuxCtl = info.dwDVAAuxCtl;
	dvinfo.dwDVAAuxSrc1 = info.dwDVAAuxSrc1;
	dvinfo.dwDVAAuxCtl1 = info.dwDVAAuxCtl1;
	dvinfo.dwDVVAuxSrc = info.dwDVVAuxSrc;
	dvinfo.dwDVVAuxCtl = info.dwDVVAuxCtl;
}


void AVI2File::setDVINFO( DVINFO &info )
{}

void AVIFile::setFccHandler( FOURCC type, FOURCC handler )
{
	for ( int i = 0; i < mainHdr.dwStreams; i++ )
	{
		if ( streamHdr[ i ].fccType == type )
		{
			int k, j = 0;
			FOURCC strf = make_fourcc( "strf" );
			BITMAPINFOHEADER bih;

			streamHdr[ i ].fccHandler = handler;

			while ( ( k = FindDirectoryEntry( strf, j++ ) ) != -1 )
			{
				ReadChunk( k, ( void* ) & bih, sizeof( BITMAPINFOHEADER ) );
				bih.biCompression = handler;
			}
		}
	}
}

bool AVIFile::getStreamFormat( void* data, FOURCC type )
{
	int i, j = 0;
	FOURCC strh = make_fourcc( "strh" );
	FOURCC strf = make_fourcc( "strf" );
	AVIStreamHeader	avi_stream_header;
	bool result = false;

	while ( ( result == false ) && ( i = FindDirectoryEntry( strh, j++ ) ) != -1 )
	{
		ReadChunk( i, ( void* ) & avi_stream_header, sizeof( AVIStreamHeader ) );
		if ( avi_stream_header.fccType == type )
		{
			FOURCC chunkID;
			int size;

			pthread_mutex_lock( &file_mutex );
			fail_neg( read( fd, &chunkID, sizeof( FOURCC ) ) );
			if ( chunkID == strf )
			{
				fail_neg( read( fd, &size, sizeof( int ) ) );
				fail_neg( read( fd, data, size ) );
				result = true;
			}
			pthread_mutex_unlock( &file_mutex );
		}
	}
	return result;
}
