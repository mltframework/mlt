/*
 * factory.h -- the factory method interfaces
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
#ifndef FACTORY_H
#define FACTORY_H

#include <pthread.h>
#include <framework/mlt.h>

typedef struct
{
	int width, height;
	int channels, frequency, samples;
	mlt_image_format iformat;
	mlt_audio_format aformat;
	struct
	{
		int audio, image, alpha;
	} shmid;
} shmem_frame_t;

#define SHMEM_QUEUE_SIZE 32
#define SHMEM_FTOK_PROJ_ID 'M'

typedef struct
{
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct
	{
		int len;
		shmem_frame_t frames[SHMEM_QUEUE_SIZE];
	} queue;
} shmem_xchg_t;

mlt_consumer consumer_shmem_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
mlt_producer producer_shmem_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

shmem_xchg_t* shmem_xchg_create( const char* arg );
int shmem_block_lookup( const void* arg );
int shmem_block_detach( const void *arg );
void* shmem_block_attach( int shmid, size_t* size );
int shmem_block_create_attach( size_t size, void** pdst );
void shmem_block_detach_destroy( const void *arg );

#endif /* FACTORY_H */
