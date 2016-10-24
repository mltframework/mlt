/*
 * factory.c -- -- the factory method interfaces
 * Copyright (C) 2016 Maksym Veremeyenko <verem@m1stereo.tv>
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

#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "factory.h"

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/shmem/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
	MLT_REGISTER( consumer_type, "shmem", consumer_shmem_init );
	MLT_REGISTER( producer_type, "shmem", producer_shmem_init );

	MLT_REGISTER_METADATA( consumer_type, "shmem", metadata, "consumer_shmem.yml" );
	MLT_REGISTER_METADATA( producer_type, "shmem", metadata, "producer_shmem.yml" );
}

void shmem_xchg_destroy(shmem_xchg_t* ctx, const char* arg)
{
	key_t k = -1;
	int shmid = 0;

	shmdt(ctx);

	if ( arg )
		k = ftok( arg, SHMEM_FTOK_PROJ_ID );
	if ( -1 != k )
		shmid = shmget(k, sizeof( shmem_xchg_t ), 0666 );
	if ( shmid > 0)
	{
		struct shmid_ds ds;
		shmctl( shmid, IPC_RMID, &ds );
	}
}

shmem_xchg_t* shmem_xchg_create(const char* arg)
{
	int f_init = 1;
	key_t k;
	int shmid;
	shmem_xchg_t* ctx;

	if ( !arg )
	{
		mlt_log_error( NULL, "%s: arg is NULL\n", __FUNCTION__ );
		return NULL;
	}

	k = ftok( arg, SHMEM_FTOK_PROJ_ID );
	if ( -1 == k )
	{
		mlt_log_error( NULL, "%s: ftok failed, errno=%d\n", __FUNCTION__, errno );
		return NULL;
	}
	mlt_log_debug( NULL, "%s:%d: key=%d, arg=[%s]\n", __FUNCTION__, __LINE__, k, arg );

	// try to get shared memory region
	shmid = shmget(k, sizeof( shmem_xchg_t ), IPC_CREAT | IPC_EXCL | 0666 );
	mlt_log_debug( NULL, "%s:%d: shmid=%d\n", __FUNCTION__, __LINE__, shmid );
	if ( -1 == shmid )
	{
		// if exist, then dont create
		if ( EEXIST == errno )
		{
			shmid = shmget(k, sizeof( shmem_xchg_t ), 0666 );
			mlt_log_debug( NULL, "%s:%d: shmid=%d\n", __FUNCTION__, __LINE__, shmid );
		}

		// once more check
		if ( -1 == shmid )
		{
			mlt_log_error( NULL, "%s: shmget failed, errno=%d\n", __FUNCTION__, errno );
			return NULL;
		}

		f_init = 0;
	}

	// attach
	ctx = (shmem_xchg_t*)shmat( shmid, NULL, 0 );
	if ( !ctx )
	{
		mlt_log_error( NULL, "%s: shmat failed, errno=%d\n", __FUNCTION__, errno );
		return NULL;
	}

	// init if needed
	if ( f_init )
	{
		pthread_mutexattr_t mattr;
		pthread_condattr_t cattr;

		memset(ctx, 0, sizeof( shmem_xchg_t ) );

		pthread_mutexattr_init( &mattr );
		pthread_mutexattr_setpshared( &mattr, PTHREAD_PROCESS_SHARED );
		pthread_mutexattr_settype( &mattr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init( &ctx->lock, &mattr );
		pthread_mutexattr_destroy( &mattr );

		pthread_condattr_init( &cattr );
		pthread_condattr_setpshared( &cattr, PTHREAD_PROCESS_SHARED );
		pthread_cond_init( &ctx->cond, &cattr );
		pthread_condattr_destroy( &cattr );
	}

	return ctx;
}

typedef struct shmem_blocks_desc
{
	void* ptr;
	int shmid;
	struct shmem_blocks_desc* next;
} shmem_blocks_t;

static pthread_mutex_t shmem_blocks_lock = PTHREAD_MUTEX_INITIALIZER;
static shmem_blocks_t* shmem_blocks_list = NULL;

static int shmem_block_find( const void* ptr, int shmid, shmem_blocks_t** pprev, shmem_blocks_t** pcurr )
{
	shmem_blocks_t* head = shmem_blocks_list;

	if ( pcurr )
		*pcurr = NULL;
	if ( pprev )
		*pprev = NULL;

	while ( head )
	{
		if ( pcurr )
			*pcurr = head;

		if ( ( ptr && (unsigned char*)ptr == (unsigned char*)head->ptr ) || ( shmid && shmid == head->shmid ) )
			return 0;

		if ( pprev )
			*pprev = head;
		head = head->next;
	}

	return -ENOENT;
}

int shmem_block_lookup( const void* arg )
{
	int r;
	shmem_blocks_t* i;

	pthread_mutex_lock( &shmem_blocks_lock );

	r = shmem_block_find( arg, 0, NULL, &i );
	if ( !r )
		r = i->shmid;

	pthread_mutex_unlock( &shmem_blocks_lock );

	return r;
}

int shmem_block_detach( const void *arg )
{
	int r;
	shmem_blocks_t *p, *c;

	pthread_mutex_lock( &shmem_blocks_lock );

	r = shmem_block_find( arg, 0, &p, &c );
	if ( !r )
	{
		if( !p )
			shmem_blocks_list = c->next;
		else
			p->next = c->next;
		shmdt( c->ptr );
		r = c->shmid;
		free( c );
	}

	pthread_mutex_unlock( &shmem_blocks_lock );

	return r;
}

void shmem_block_detach_destroy( const void *arg )
{
	int r = shmem_block_detach ( arg );

	if ( r > 0 )
	{
		struct shmid_ds ds;
		shmctl( r, IPC_RMID, &ds );
	}
}

void* shmem_block_attach( int shmid, size_t* psize )
{
	void* ptr = NULL;
	shmem_blocks_t *c;
	struct shmid_ds ds;

	c = (shmem_blocks_t*) calloc( 1, sizeof( shmem_blocks_t ) );
	ptr = c->ptr = shmat( c->shmid = shmid, NULL, 0 );

	pthread_mutex_lock( &shmem_blocks_lock );

	c->next = shmem_blocks_list;
	shmem_blocks_list = c;

	pthread_mutex_unlock( &shmem_blocks_lock );

	shmctl( shmid, IPC_STAT, &ds );
	*psize = ds.shm_segsz;

	return ptr;
}

int shmem_block_create_attach( size_t size, void** pdst )
{
	int r;

	r = shmget( IPC_PRIVATE, size, IPC_CREAT );

	*pdst = shmem_block_attach( r, &size );

	return r;
}
