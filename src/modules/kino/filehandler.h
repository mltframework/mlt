/*
* filehandler.h
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

#ifndef _FILEHANDLER_H
#define _FILEHANDLER_H

// enum { PAL_FORMAT, NTSC_FORMAT, AVI_DV1_FORMAT, AVI_DV2_FORMAT, QT_FORMAT, RAW_FORMAT, TEST_FORMAT, UNDEFINED };

#include <vector>
using std::vector;

#include <string>
using std::string;

#include "riff.h"
#include "avi.h"
#include <sys/types.h>
#include <stdint.h>

enum { AVI, PLAYLIST, RAW_DV, QT, UNKNOWN_FORMAT };
enum { PAL_FORMAT, NTSC_FORMAT, AVI_DV1_FORMAT, AVI_DV2_FORMAT, QT_FORMAT, RAW_FORMAT, TEST_FORMAT, UNDEFINED };
enum { DISPLAY_XX, DISPLAY_GDKRGB, DISPLAY_GDKRGB32, DISPLAY_XV, DISPLAY_SDL };

enum { NORM_UNSPECIFIED=0, NORM_PAL=1, NORM_NTSC=2 };
enum { AUDIO_32KHZ=0, AUDIO_44KHZ=1, AUDIO_48KHZ=2 };
enum { ASPECT_43=0, ASPECT_169=1 };

enum FileCaptureMode {
    CAPTURE_IGNORE,
    CAPTURE_FRAME_APPEND,
    CAPTURE_FRAME_INSERT,
    CAPTURE_MOVIE_APPEND
};

class FileTracker
{
protected:
	FileTracker();
	~FileTracker();
public:
	static FileTracker &GetInstance( );
	void SetMode( FileCaptureMode );
	FileCaptureMode GetMode( );
	unsigned int Size();
	char *Get( int );
	void Add( const char * );
	void Clear( );
private:
	static FileTracker *instance;
	vector <char *> list;
	FileCaptureMode mode;
};

class FileHandler
{
public:

	FileHandler();
	virtual ~FileHandler();

	virtual bool GetAutoSplit() const;
	virtual bool GetTimeStamp() const;
	virtual string GetBaseName() const;
	virtual string GetExtension() const;
	virtual int GetMaxFrameCount() const;
	virtual off_t GetMaxFileSize() const;
	virtual off_t GetFileSize() = 0;
	virtual int GetTotalFrames() = 0;
	virtual string GetFilename() const;

	virtual void SetAutoSplit( bool );
	virtual void SetTimeStamp( bool );
	virtual void SetBaseName( const string& base );
	virtual void SetMaxFrameCount( int );
	virtual void SetEveryNthFrame( int );
	virtual void SetMaxFileSize( off_t );
	//virtual void SetSampleFrame( const Frame& sample );

	//virtual bool WriteFrame( const Frame& frame );
	virtual bool FileIsOpen() = 0;
	virtual bool Create( const string& filename ) = 0;
	//virtual int Write( const Frame& frame ) = 0;
	virtual int Close() = 0;
	virtual bool Done( void );

	virtual bool Open( const char *s ) = 0;
	virtual int GetFrame( uint8_t *data, int frameNum ) = 0;
	int GetFramesWritten() const
	{
		return framesWritten;
	}

protected:
	bool done;
	bool autoSplit;
	bool timeStamp;
	int maxFrameCount;
	int framesWritten;
	int everyNthFrame;
	int framesToSkip;
	off_t maxFileSize;
	string base;
	string extension;
	string filename;
};


class RawHandler: public FileHandler
{
public:
	int fd;

	RawHandler();
	~RawHandler();

	bool FileIsOpen();
	bool Create( const string& filename );
	//int Write( const Frame& frame );
	int Close();
	off_t GetFileSize();
	int GetTotalFrames();
	bool Open( const char *s );
	int GetFrame( uint8_t *data, int frameNum );
private:
	int numBlocks;
};


class AVIHandler: public FileHandler
{
public:
	AVIHandler( int format = AVI_DV1_FORMAT );
	~AVIHandler();

	//void SetSampleFrame( const Frame& sample );
	bool FileIsOpen();
	bool Create( const string& filename );
	//int Write( const Frame& frame );
	int Close();
	off_t GetFileSize();
	int GetTotalFrames();
	bool Open( const char *s );
	int GetFrame( uint8_t *data, int frameNum );
	bool GetOpenDML() const;
	void SetOpenDML( bool );
	int GetFormat() const
	{
		return aviFormat;
	}

protected:
	AVIFile *avi;
	int aviFormat;
	//AudioInfo audioInfo;
	//VideoInfo videoInfo;
	bool isOpenDML;
	DVINFO dvinfo;
	FOURCC	fccHandler;
	int channels;
	bool isFullyInitialized;
	int16_t *audioBuffer;
	int16_t *audioChannels[ 4 ];
};


#ifdef HAVE_LIBQUICKTIME
#include <lqt.h>

class QtHandler: public FileHandler
{
public:
	QtHandler();
	~QtHandler();

	bool FileIsOpen();
	bool Create( const string& filename );
	//int Write( const Frame& frame );
	int Close();
	off_t GetFileSize();
	int GetTotalFrames();
	bool Open( const char *s );
	int GetFrame( uint8_t *data, int frameNum );
	void AllocateAudioBuffers();

private:
	quicktime_t *fd;
	long samplingRate;
	int samplesPerBuffer;
	int channels;
	bool isFullyInitialized;
	unsigned int audioBufferSize;
	int16_t *audioBuffer;
	short int** audioChannelBuffer;

	void Init();
	inline void DeinterlaceStereo16( void* pInput, int iBytes, void* pLOutput, void* pROutput );

};
#endif

#endif
