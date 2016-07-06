/*
* avi.h library for AVI file format i/o
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

/** Common AVI declarations
 
    Some of this comes from the public domain AVI specification, which
    explains the microsoft-style definitions.
 
    \file avi.h
*/

#ifndef _AVI_H
#define _AVI_H 1

#include <stdint.h>
#include "riff.h"

#define PACKED(x)	__attribute__((packed)) x

#define AVI_SMALL_INDEX (0x01)
#define AVI_LARGE_INDEX (0x02)
#define KINO_AVI_INDEX_OF_INDEXES (0x00)
#define KINO_AVI_INDEX_OF_CHUNKS (0x01)
#define AVI_INDEX_2FIELD (0x01)

enum { AVI_PAL, AVI_NTSC, AVI_AUDIO_48KHZ, AVI_AUDIO_44KHZ, AVI_AUDIO_32KHZ };

/** Declarations of the main AVI file header
 
    The contents of this struct goes into the 'avih' chunk.  */

typedef struct
{
	/// frame display rate (or 0L)
	DWORD dwMicroSecPerFrame;

	/// max. transfer rate
	DWORD dwMaxBytesPerSec;

	/// pad to multiples of this size, normally 2K
	DWORD dwPaddingGranularity;

	/// the ever-present flags
	DWORD dwFlags;

	/// # frames in file
	DWORD dwTotalFrames;
	DWORD dwInitialFrames;
	DWORD dwStreams;
	DWORD dwSuggestedBufferSize;

	DWORD dwWidth;
	DWORD dwHeight;

	DWORD dwReserved[ 4 ];
}
PACKED(MainAVIHeader);

typedef struct
{
	WORD top, bottom, left, right;
}
PACKED(RECT);

/** Declaration of a stream header
 
    The contents of this struct goes into the 'strh' header. */

typedef struct
{
	FOURCC fccType;
	FOURCC fccHandler;
	DWORD dwFlags;                /* Contains AVITF_* flags */
	WORD wPriority;
	WORD wLanguage;
	DWORD dwInitialFrames;
	DWORD dwScale;
	DWORD dwRate;                 /* dwRate / dwScale == samples/second */
	DWORD dwStart;
	DWORD dwLength;               /* In units above... */
	DWORD dwSuggestedBufferSize;
	DWORD dwQuality;
	DWORD dwSampleSize;
	RECT rcFrame;
}
PACKED(AVIStreamHeader);

typedef struct
{
	DWORD dwDVAAuxSrc;
	DWORD dwDVAAuxCtl;
	DWORD dwDVAAuxSrc1;
	DWORD dwDVAAuxCtl1;
	DWORD dwDVVAuxSrc;
	DWORD dwDVVAuxCtl;
	DWORD dwDVReserved[ 2 ];
}
PACKED(DVINFO);

typedef struct
{
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
	char dummy[ 1040 ];
}
PACKED(BITMAPINFOHEADER);

typedef struct
{
	WORD wFormatTag;
	WORD nChannels;
	DWORD nSamplesPerSec;
	DWORD nAvgBytesPerSec;
	WORD nBlockAlign;
	WORD wBitsPerSample;
	WORD cbSize;
	WORD dummy;
}
PACKED(WAVEFORMATEX);

typedef struct
{
	WORD wLongsPerEntry;
	BYTE bIndexSubType;
	BYTE bIndexType;
	DWORD nEntriesInUse;
	FOURCC dwChunkId;
	DWORD dwReserved[ 3 ];
	struct avisuperindex_entry
	{
		QUADWORD qwOffset;
		DWORD dwSize;
		DWORD dwDuration;
	}
	aIndex[ 3198 ];
}
PACKED(AVISuperIndex);

typedef struct
{
	WORD wLongsPerEntry;
	BYTE bIndexSubType;
	BYTE bIndexType;
	DWORD nEntriesInUse;
	FOURCC dwChunkId;
	QUADWORD qwBaseOffset;
	DWORD dwReserved;
	struct avifieldindex_entry
	{
		DWORD dwOffset;
		DWORD dwSize;
	}
	aIndex[ 17895 ];
}
PACKED(AVIStdIndex);

typedef struct
{
	struct avisimpleindex_entry
	{
		FOURCC	dwChunkId;
		DWORD	dwFlags;
		DWORD	dwOffset;
		DWORD	dwSize;
	}
	aIndex[ 20000 ];
	DWORD	nEntriesInUse;
}
PACKED(AVISimpleIndex);

typedef struct
{
	DWORD dirEntryType;
	DWORD dirEntryName;
	DWORD dirEntryLength;
	size_t dirEntryOffset;
	int dirEntryWrittenFlag;
	int dirEntryParentList;
}
AviDirEntry;


/** base class for all AVI type files
 
    It contains methods and members which are the same in all AVI type files regardless of the particular compression, number
    of streams etc. 
 
    The AVIFile class also contains methods for handling several indexes to the video frame content. */

class AVIFile : public RIFFFile
{
public:
	AVIFile();
	AVIFile( const AVIFile& );
	virtual ~AVIFile();
	virtual AVIFile& operator=( const AVIFile& );

	virtual void Init( int format, int sampleFrequency, int indexType );
	virtual int GetDVFrameInfo( off_t &offset, int &size, int frameNum );
	virtual int GetFrameInfo( off_t &offset, int &size, int frameNum, FOURCC chunkID );
	virtual int GetDVFrame( uint8_t *data, int frameNum );
	virtual int getFrame( void *data, int frameNum, FOURCC chunkID );
	virtual int GetTotalFrames() const;
	virtual void PrintDirectoryEntryData( const RIFFDirEntry &entry ) const;
	//virtual bool WriteFrame( const Frame &frame ) { return false; }
	virtual void ParseList( int parent );
	virtual void ParseRIFF( void );
	virtual void ReadIndex( void );
	virtual void WriteRIFF( void )
	{ }
	virtual void FlushIndx( int stream );
	virtual void UpdateIndx( int stream, int chunk, int duration );
	virtual void UpdateIdx1( int chunk, int flags );
	virtual bool verifyStreamFormat( FOURCC type );
	virtual bool verifyStream( FOURCC type );
	virtual bool isOpenDML( void );
	virtual void setDVINFO( DVINFO& )
	{ }
	virtual void setFccHandler( FOURCC type, FOURCC handler );
	virtual bool getStreamFormat( void* data, FOURCC type );

protected:
	MainAVIHeader mainHdr;
	AVISimpleIndex *idx1;
	int file_list;
	int riff_list;
	int hdrl_list;
	int avih_chunk;
	int movi_list;
	int junk_chunk;
	int idx1_chunk;

	AVIStreamHeader streamHdr[ 2 ];
	AVISuperIndex *indx[ 2 ];
	AVIStdIndex *ix[ 2 ];
	int indx_chunk[ 2 ];
	int ix_chunk[ 2 ];
	int strl_list[ 2 ];
	int strh_chunk[ 2 ];
	int strf_chunk[ 2 ];

	int index_type;
	int current_ix00;

	DWORD dmlh[ 62 ];
	int odml_list;
	int dmlh_chunk;
	bool isUpdateIdx1;

};


/** writing Type 1 DV AVIs
 
*/

class AVI1File : public AVIFile
{
public:
	AVI1File();
	virtual ~AVI1File();

	virtual void Init( int format, int sampleFrequency, int indexType );
	//virtual bool WriteFrame( const Frame &frame );
	virtual void WriteRIFF( void );
	virtual void setDVINFO( DVINFO& );

private:
	DVINFO dvinfo;

	AVI1File( const AVI1File& );
	AVI1File& operator=( const AVI1File& );
};


/** writing Type 2 (separate audio data) DV AVIs
 
This file type contains both audio and video tracks. It is therefore more compatible
to certain Windows programs, which expect any AVI having both audio and video tracks.
The video tracks contain the raw DV data (as in type 1) and the extracted audio tracks.
 
Note that because the DV data contains audio information anyway, this means duplication
of data and a slight increase of file size.
 
*/

class AVI2File : public AVIFile
{
public:
	AVI2File();
	virtual ~AVI2File();

	virtual void Init( int format, int sampleFrequency, int indexType );
	//virtual bool WriteFrame( const Frame &frame );
	virtual void WriteRIFF( void );
	virtual void setDVINFO( DVINFO& );

private:
	BITMAPINFOHEADER bitmapinfo;
	WAVEFORMATEX waveformatex;

	AVI2File( const AVI2File& );
	AVI2File& operator=( const AVI2File& );
};
#endif
