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
* Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*
* Tag: $Name$
*
* Change log:
* 
* $Log$
* Revision 1.2  2005/04/15 14:37:03  lilo_booter
* Minor correction
*
* Revision 1.1  2005/04/15 14:28:26  lilo_booter
* Initial version
*
* Revision 1.16  2005/04/01 23:43:10  ddennedy
* apply endian fixes from Daniel Kobras
*
* Revision 1.15  2004/10/11 01:37:11  ddennedy
* mutex safety locks in RIFF and AVI classes, type 2 AVI optimization, mencoder export script
*
* Revision 1.14  2003/11/25 23:00:52  ddennedy
* cleanup and a few bugfixes
*
* Revision 1.13  2003/10/21 16:34:32  ddennedy
* GNOME2 port phase 1: initial checkin
*
* Revision 1.11.2.5  2003/07/24 14:13:57  ddennedy
* support for distinct audio stream in type2 AVI and Quicktime; support for more DV FOURCCs
*
* Revision 1.11.2.4  2003/06/10 23:53:36  ddennedy
* Daniel Kobras' WriteFrame error handling and automatic OpenDML, bugfixes in scene list updates, export AV/C Record
*
* Revision 1.11.2.3  2003/02/20 21:59:57  ddennedy
* bugfixes to capture and AVI
*
* Revision 1.11.2.2  2003/01/13 05:15:31  ddennedy
* added More Info panel and supporting methods
*
* Revision 1.11.2.1  2002/11/25 04:48:31  ddennedy
* bugfix to report errors when loading files
*
* Revision 1.11  2002/10/08 07:46:41  ddennedy
* AVI bugfixes, compatibility, optimization, warn bad file in capture and export dv file, allow no mplex
*
* Revision 1.10  2002/05/17 08:04:25  ddennedy
* revert const-ness of Frame references in Frame, FileHandler, and AVI classes
*
* Revision 1.9  2002/05/15 04:39:35  ddennedy
* bugfixes to dv2 AVI write, audio export, Xv init
*
* Revision 1.8  2002/04/29 05:09:22  ddennedy
* raw dv file support, Frame::ExtractAudio uses libdv, audioScrub prefs
*
* Revision 1.7  2002/04/09 06:53:42  ddennedy
* cleanup, new libdv 0.9.5, large AVI, dnd storyboard
*
* Revision 1.7  2002/03/25 21:34:25  arne
* Support for large (64 bit) files mostly completed
*
* Revision 1.6  2002/03/10 13:29:41  arne
* more changes for 64 bit access
*
* Revision 1.5  2002/03/09 17:59:28  arne
* moved index routines to AVIFile
*
* Revision 1.4  2002/03/09 10:26:26  arne
* improved constructors and assignment operator
*
* Revision 1.3  2002/03/09 08:55:57  arne
* moved a few variables to AVIFile
*
* Revision 1.2  2002/03/04 19:22:43  arne
* updated to latest Kino avi code
*
* Revision 1.1.1.1  2002/03/03 19:08:08  arne
* import of version 1.01
*
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
#define AVI_INDEX_OF_INDEXES (0x00)
#define AVI_INDEX_OF_CHUNKS (0x01)
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
	aIndex[ 2014 ];
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
	aIndex[ 4028 ];
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
