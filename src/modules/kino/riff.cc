/*
* riff.cc library for RIFF file format i/o
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

#include <string> 
//#include <stdio.h>
#include <iostream>
#include <iomanip>

using std::cout;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;
using std::endl;

// C includes

#include <fcntl.h>
#include <unistd.h>
#include <assert.h>

// local includes

#include "error.h"
#include "riff.h"


/** make a 32 bit "string-id"
 
    \param s a pointer to 4 chars
    \return the 32 bit "string id"
    \bugs It is not checked whether we really have 4 characters
 
    Some compilers understand constants like int id = 'ABCD'; but I
    could not get it working on the gcc compiler so I had to use this
    workaround. We can now use id = make_fourcc("ABCD") instead. */

FOURCC make_fourcc( const char *s )
{
	if ( s[ 0 ] == 0 )
		return 0;
	else
		return *( ( FOURCC* ) s );
}


RIFFDirEntry::RIFFDirEntry()
{
	type = 0;
	name = 0;
	length = 0;
	offset = 0;
	parent = 0;
	written = 0;
}


RIFFDirEntry::RIFFDirEntry ( FOURCC t, FOURCC n, int l, int o, int p ) : type( t ), name( n ), length( l ), offset( o ), parent( p ), written( 0 )
{}


/** Creates the object without an output file.
 
*/

RIFFFile::RIFFFile() : fd( -1 )
{
	pthread_mutex_init( &file_mutex, NULL );
}


/* Copy constructor
 
   Duplicate the file descriptor
*/

RIFFFile::RIFFFile( const RIFFFile& riff ) : fd( -1 )
{
	if ( riff.fd != -1 )
	{
		fd = dup( riff.fd );
	}
	directory = riff.directory;
}


/** Destroys the object.
 
    If it has an associated opened file, close it. */

RIFFFile::~RIFFFile()
{
	Close();
	pthread_mutex_destroy( &file_mutex );
}


RIFFFile& RIFFFile::operator=( const RIFFFile& riff )
{
	if ( fd != riff.fd )
	{
		Close();
		if ( riff.fd != -1 )
		{
			fd = dup( riff.fd );
		}
		directory = riff.directory;
	}
	return *this;
}


/** Creates or truncates the file.
 
    \param s the filename
*/

bool RIFFFile::Create( const char *s )
{
	fd = open( s, O_RDWR | O_NONBLOCK | O_CREAT | O_TRUNC, 00644 );

	if ( fd == -1 )
		return false;
	else
		return true;
}


/** Opens the file read only.
 
    \param s the filename
*/

bool RIFFFile::Open( const char *s )
{
	fd = open( s, O_RDONLY | O_NONBLOCK );

	if ( fd == -1 )
		return false;
	else
		return true;
}


/** Destroys the object.
 
    If it has an associated opened file, close it. */

void RIFFFile::Close()
{
	if ( fd != -1 )
	{
		close( fd );
		fd = -1;
	}
}


/** Adds an entry to the list of containers.
 
    \param type the type of this entry
    \param name the name
    \param length the length of the data in the container
    \param list the container in which this object is contained. 
    \return the ID of the newly created entry
 
    The topmost object is not contained in any other container. Use
    the special ID RIFF_NO_PARENT to create the topmost object. */

int RIFFFile::AddDirectoryEntry( FOURCC type, FOURCC name, off_t length, int list )
{
	/* Put all parameters in an RIFFDirEntry object. The offset is
	   currently unknown. */

	RIFFDirEntry entry( type, name, length, 0 /* offset */, list );

	/* If the new chunk is in a list, then get the offset and size
	   of that list. The offset of this chunk is the end of the list
	   (parent_offset + parent_length) plus the size of the chunk
	   header. */

	if ( list != RIFF_NO_PARENT )
	{
		RIFFDirEntry parent = GetDirectoryEntry( list );
		entry.offset = parent.offset + parent.length + RIFF_HEADERSIZE;
	}

	/* The list which this new chunk is a member of has now increased in
	   size. Get that directory entry and bump up its length by the size
	   of the chunk. Since that list may also be contained in another
	   list, walk up to the top of the tree. */

	while ( list != RIFF_NO_PARENT )
	{
		RIFFDirEntry parent = GetDirectoryEntry( list );
		parent.length += RIFF_HEADERSIZE + length;
		SetDirectoryEntry( list, parent );
		list = parent.parent;
	}

	directory.insert( directory.end(), entry );

	return directory.size() - 1;
}


/** Modifies an entry.
 
    \param i the ID of the entry which is to modify
    \param type the type of this entry
    \param name the name
    \param length the length of the data in the container
    \param list the container in which this object is contained.
    \note Do not change length, offset, or the parent container.
    \note Do not change an empty name ("") to a name and vice versa */

void RIFFFile::SetDirectoryEntry( int i, FOURCC type, FOURCC name, off_t length, off_t offset, int list )
{
	RIFFDirEntry entry( type, name, length, offset, list );

	assert( i >= 0 && i < ( int ) directory.size() );

	directory[ i ] = entry;
}


/** Modifies an entry.
 
    The entry.written flag is set to false because the contents has been modified
 
    \param i the ID of the entry which is to modify
    \param entry the new entry 
    \note Do not change length, offset, or the parent container.
    \note Do not change an empty name ("") to a name and vice versa */

void RIFFFile::SetDirectoryEntry( int i, RIFFDirEntry &entry )
{
	assert( i >= 0 && i < ( int ) directory.size() );

	entry.written = false;
	directory[ i ] = entry;
}


/** Retrieves an entry.
 
    Gets the most important member variables.
 
    \param i the ID of the entry to retrieve
    \param type
    \param name
    \param length
    \param offset
    \param list */

void RIFFFile::GetDirectoryEntry( int i, FOURCC &type, FOURCC &name, off_t &length, off_t &offset, int &list ) const
{
	RIFFDirEntry entry;

	assert( i >= 0 && i < ( int ) directory.size() );

	entry = directory[ i ];
	type = entry.type;
	name = entry.name;
	length = entry.length;
	offset = entry.offset;
	list = entry.parent;
}


/** Retrieves an entry.
 
    Gets the whole RIFFDirEntry object.
 
    \param i the ID of the entry to retrieve
    \return the entry */

RIFFDirEntry RIFFFile::GetDirectoryEntry( int i ) const
{
	assert( i >= 0 && i < ( int ) directory.size() );

	return directory[ i ];
}


/** Calculates the total size of the file
 
    \return the size the file in bytes
*/

off_t RIFFFile::GetFileSize( void ) const
{

	/* If we have at least one entry, return the length field
	   of the FILE entry, which is the length of its contents,
	   which is the actual size of whatever is currently in the
	   AVI directory structure. 

	   Note that the first entry does not belong to the AVI
	   file.

	   If we don't have any entry, the file size is zero. */

	if ( directory.size() > 0 )
		return directory[ 0 ].length;
	else
		return 0;
}


/** prints the attributes of the entry
 
    \param i the ID of the entry to print
*/

void RIFFFile::PrintDirectoryEntry ( int i ) const
{
	RIFFDirEntry entry;
	RIFFDirEntry parent;
	FOURCC entry_name;
	FOURCC list_name;

	/* Get all attributes of the chunk object. If it is contained
	   in a list, get the name of the list too (otherwise the name of
	   the list is blank). If the chunk object doesn´t have a name (only
	   LISTs and RIFFs have a name), the name is blank. */

	entry = GetDirectoryEntry( i );
	if ( entry.parent != RIFF_NO_PARENT )
	{
		parent = GetDirectoryEntry( entry.parent );
		list_name = parent.name;
	}
	else
	{
		list_name = make_fourcc( "    " );
	}
	if ( entry.name != 0 )
	{
		entry_name = entry.name;
	}
	else
	{
		entry_name = make_fourcc( "    " );
	}

	/* Print out the ascii representation of type and name, as well as
	   length and file offset. */

	cout << hex << setfill( '0' ) << "type: "
	<< ((char *)&entry.type)[0]
	<< ((char *)&entry.type)[1]
	<< ((char *)&entry.type)[2]
	<< ((char *)&entry.type)[3]
	<< " name: "
	<< ((char *)&entry_name)[0]
	<< ((char *)&entry_name)[1]
	<< ((char *)&entry_name)[2]
	<< ((char *)&entry_name)[3]
	<< " length: 0x" << setw( 12 ) << entry.length
	<< " offset: 0x" << setw( 12 ) << entry.offset
	<< " list: "
	<< ((char *)&list_name)[0]
	<< ((char *)&list_name)[1]
	<< ((char *)&list_name)[2]
	<< ((char *)&list_name)[3] << dec << endl;

	/* print the content itself */

	PrintDirectoryEntryData( entry );
}


/** prints the contents of the entry
 
    Prints a readable representation of the contents of an index.
    Override this to print out any objects you store in the RIFF file.
 
    \param entry the entry to print */

void RIFFFile::PrintDirectoryEntryData( const RIFFDirEntry &entry ) const
	{}


/** prints the contents of the whole directory
 
    Prints a readable representation of the contents of an index.
    Override this to print out any objects you store in the RIFF file.
 
    \param entry the entry to print */

void RIFFFile::PrintDirectory() const
{
	int i;
	int count = directory.size();

	for ( i = 0; i < count; ++i )
		PrintDirectoryEntry( i );
}


/** finds the index
 
    finds the index of a given directory entry type 
 
    \todo inefficient if the directory has lots of items
    \param type the type of the entry to find
    \param n    the zero-based instance of type to locate
    \return the index of the found object in the directory, or -1 if not found */

int RIFFFile::FindDirectoryEntry ( FOURCC type, int n ) const
{
	int i, j = 0;
	int count = directory.size();

	for ( i = 0; i < count; ++i )
		if ( directory[ i ].type == type )
		{
			if ( j == n )
				return i;
			j++;
		}

	return -1;
}


/** Reads all items that are contained in one list
 
    Read in one chunk and add it to the directory. If the chunk
    happens to be of type LIST, then call ParseList recursively for
    it.
 
    \param parent The id of the item to process
*/

void RIFFFile::ParseChunk( int parent )
{
	FOURCC type;
	DWORD length;
	int typesize;

	/* Check whether it is a LIST. If so, let ParseList deal with it */

	fail_if( read( fd, &type, sizeof( type ) ) != sizeof( type ));
	if ( type == make_fourcc( "LIST" ) )
	{
		typesize = (int) -sizeof( type );
		fail_if( lseek( fd, typesize, SEEK_CUR ) == ( off_t ) - 1 );
		ParseList( parent );
	}

	/* it is a normal chunk, create a new directory entry for it */

	else
	{
		fail_neg( read( fd, &length, sizeof( length ) ) );
		if ( length & 1 )
			length++;
		AddDirectoryEntry( type, 0, length, parent );
		fail_if( lseek( fd, length, SEEK_CUR ) == ( off_t ) - 1 );
	}
}


/** Reads all items that are contained in one list
 
    \param parent The id of the list to process
 
*/

void RIFFFile::ParseList( int parent )
{
	FOURCC type;
	FOURCC name;
	int list;
	DWORD length;
	off_t pos;
	off_t	listEnd;

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


/** Reads the directory structure of the whole RIFF file
 
*/

void RIFFFile::ParseRIFF( void )
{
	FOURCC type;
	DWORD length;
	off_t filesize = 0;
	off_t pos;
	int container = AddDirectoryEntry( make_fourcc( "FILE" ), make_fourcc( "FILE" ), 0, RIFF_NO_PARENT );

	pos = lseek( fd, 0, SEEK_SET );
	fail_if( pos == -1 );

	/* calculate file size from RIFF header instead from physical file. */

	while ( ( read( fd, &type, sizeof( type ) ) > 0 ) &&
	        ( read( fd, &length, sizeof( length ) ) > 0 ) &&
	        ( type == make_fourcc( "RIFF" ) ) )
	{

		filesize += length + RIFF_HEADERSIZE;

		fail_if( lseek( fd, pos, SEEK_SET ) == ( off_t ) - 1 );
		ParseList( container );
		pos = lseek( fd, 0, SEEK_CUR );
		fail_if( pos == ( off_t ) - 1 );
	}
}


/** Reads one item including its contents from the RIFF file
 
    \param chunk_index The index of the item to write
    \param data A pointer to the data
 
*/

void RIFFFile::ReadChunk( int chunk_index, void *data, off_t data_len )
{
	RIFFDirEntry entry;

	entry = GetDirectoryEntry( chunk_index );
	pthread_mutex_lock( &file_mutex );
	fail_if( lseek( fd, entry.offset, SEEK_SET ) == ( off_t ) - 1 );
	fail_neg( read( fd, data, entry.length > data_len ? data_len : entry.length ) );
	pthread_mutex_unlock( &file_mutex );
}


/** Writes one item including its contents to the RIFF file
 
    \param chunk_index The index of the item to write
    \param data A pointer to the data
 
*/

void RIFFFile::WriteChunk( int chunk_index, const void *data )
{
	RIFFDirEntry entry;

	entry = GetDirectoryEntry( chunk_index );
	pthread_mutex_lock( &file_mutex );
	fail_if( lseek( fd, entry.offset - RIFF_HEADERSIZE, SEEK_SET ) == ( off_t ) - 1 );
	fail_neg( write( fd, &entry.type, sizeof( entry.type ) ) );
	DWORD length = entry.length;
	fail_neg( write( fd, &length, sizeof( length ) ) );
	fail_neg( write( fd, data, entry.length ) );
	pthread_mutex_unlock( &file_mutex );

	/* Remember that this entry already has been written. */

	directory[ chunk_index ].written = true;
}


/** Writes out the directory structure
 
    For all items in the directory list that have not been written
    yet, it seeks to the file position where that item should be
    stored and writes the type and length field. If the item has a
    name, it will also write the name field.
 
    \note It does not write the contents of any item. Use WriteChunk to do that. */

void RIFFFile::WriteRIFF( void )
{
	int i;
	RIFFDirEntry entry;
	int count = directory.size();

	/* Start at the second entry (RIFF), since the first entry (FILE)
	   is needed only for internal purposes and is not written to the
	   file. */

	for ( i = 1; i < count; ++i )
	{

		/* Only deal with entries that haven´t been written */

		entry = GetDirectoryEntry( i );
		if ( entry.written == false )
		{

			/* A chunk entry consist of its type and length, a list
			   entry has an additional name. Look up the entry, seek
			   to the start of the header, which is at the offset of
			   the data start minus the header size and write out the
			   items. */

			fail_if( lseek( fd, entry.offset - RIFF_HEADERSIZE, SEEK_SET ) == ( off_t ) - 1 ) ;
			fail_neg( write( fd, &entry.type, sizeof( entry.type ) ) );
			DWORD length = entry.length;
			fail_neg( write( fd, &length, sizeof( length ) ) );

			/* If it has a name, it is a list. Write out the extra name
			   field. */

			if ( entry.name != 0 )
			{
				fail_neg( write( fd, &entry.name, sizeof( entry.name ) ) );
			}

			/* Remember that this entry already has been written. */

			directory[ i ].written = true;
		}
	}
}
