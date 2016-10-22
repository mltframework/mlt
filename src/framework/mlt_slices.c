/**
 * \file mlt_slices.c
 * \brief sliced threading processing helper
 * \see mlt_slices_s
 *
 * Copyright (C) 2016 Meltytech, LLC
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mlt_slices.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#ifdef _WIN32
#include <windows.h>
#endif
#define MAX_SLICES 32
#define ENV_SLICES "MLT_SLICES_COUNT"

struct mlt_slices_s
{
	int f_exit;
	int count;
	int readys;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond_var_job;
	pthread_cond_t cond_var_ready;
	pthread_t threads[MAX_SLICES];
	struct
	{
		int jobs, done;
		mlt_slices_proc proc;
		void* cookie;
	} runtime;
};

static void* mlt_slices_worker( void* p )
{
	int id, idx;
	mlt_slices ctx = (mlt_slices)p;

	while ( 1 )
	{
		if ( ctx->f_exit )
			pthread_exit(0);

		pthread_mutex_lock( &ctx->cond_mutex );
		id = ctx->readys;
		ctx->readys++;
		pthread_cond_signal( &ctx->cond_var_ready );
		if ( !ctx->f_exit )
			pthread_cond_wait( &ctx->cond_var_job, &ctx->cond_mutex );
		pthread_mutex_unlock( &ctx->cond_mutex );

		if ( ctx->f_exit )
			pthread_exit(0);

		pthread_mutex_lock( &ctx->cond_mutex );
		while ( ctx->runtime.done < ctx->runtime.jobs && !ctx->f_exit )
		{
			idx = ctx->runtime.done;
			ctx->runtime.done++;
			pthread_mutex_unlock( &ctx->cond_mutex );
			ctx->runtime.proc( id, idx, ctx->runtime.jobs, ctx->runtime.cookie );
			pthread_mutex_lock( &ctx->cond_mutex );
		}
		pthread_mutex_unlock( &ctx->cond_mutex );
	}

	return NULL;
}

/** Initialize a sliced threading context
 *
 * \public \memberof mlt_slices_s
 * \param threads number of threads to use for job list
 * \param policy scheduling policy of processing threads
 * \param priority priority value that can be used with the scheduling algorithm
 * \return the context pointer
 */

mlt_slices mlt_slices_init( int threads, int policy, int priority )
{
	pthread_attr_t tattr;
	struct sched_param param;
	mlt_slices ctx = (mlt_slices)calloc( 1, sizeof( struct mlt_slices_s ) );
	char *env = getenv( ENV_SLICES );
#ifdef _WIN32
	int cpus = GetCurrentProcessorNumber( );
#else
	int cpus = sysconf( _SC_NPROCESSORS_ONLN );
#endif
	int i, env_val = env ? atoi(env) : 0;

	/* check given threads count */
	if ( !env || !env_val )
	{
		if ( threads < 0 )
			threads = -threads * cpus;
		else if ( !threads )
			threads = cpus;
	}
	else if ( env_val < 0 )
	{
		if ( threads < 0 )
			threads = env_val * threads * cpus;
		else if ( !threads )
			threads = -env_val * cpus;
		else
			threads = -env_val * threads;
	}
	else // env_val > 0
	{
		if ( threads < 0 )
			threads = env_val * threads;
		else if ( !threads )
			threads = env_val;
		else
			threads = threads;
	}
	if ( threads > MAX_SLICES )
		threads = MAX_SLICES;

	ctx->count = threads;

	/* init attributes */
	pthread_mutex_init ( &ctx->cond_mutex, NULL );
	pthread_cond_init ( &ctx->cond_var_job, NULL );
	pthread_cond_init ( &ctx->cond_var_ready, NULL );
	pthread_attr_init( &tattr );
	pthread_attr_setschedpolicy( &tattr, policy );
	param.sched_priority = priority;
	pthread_attr_setschedparam( &tattr, &param );

	/* run worker threads */
	for ( i = 0; i < ctx->count; i++ )
	{
		pthread_create( &ctx->threads[i], &tattr, mlt_slices_worker, ctx );
		pthread_setschedparam( ctx->threads[i], policy, &param);
	}

	pthread_attr_destroy( &tattr );

	/* ready wait workers */
	pthread_mutex_lock( &ctx->cond_mutex );
	while ( ctx->readys != ctx->count )
		pthread_cond_wait( &ctx->cond_var_ready, &ctx->cond_mutex );
	pthread_mutex_unlock( &ctx->cond_mutex );

	/* return context */
	return ctx;
}

/** Destroy sliced threading context
 *
 * \public \memberof mlt_slices_s
 * \param ctx context pointer
 */

void mlt_slices_close( mlt_slices ctx )
{
	int j;

	/* notify to exit */
	ctx->f_exit = 1;
	pthread_mutex_lock( &ctx->cond_mutex );
	pthread_cond_broadcast( &ctx->cond_var_job);
	pthread_mutex_unlock( &ctx->cond_mutex );

	/* wait for threads exit */
	for ( j = 0; j < ctx->count; j++ )
		pthread_join ( ctx->threads[j], NULL );

	/* destroy vars */
	pthread_cond_destroy ( &ctx->cond_var_ready );
	pthread_cond_destroy ( &ctx->cond_var_job );
	pthread_mutex_destroy ( &ctx->cond_mutex );

	/* free context */
	free ( ctx );
}

/** Run sliced execution
 *
 * \public \memberof mlt_slices_s
 * \param ctx context pointer
 * \param jobs number of jobs to proccess
 * \param proc number of jobs to proccess
 */

void mlt_slices_run( mlt_slices ctx, int jobs, mlt_slices_proc proc, void* cookie )
{
	/* check jobs count */
	if ( jobs < 0 )
		jobs = (-jobs) * ctx->count;
	if ( !jobs )
		jobs = ctx->count;

	/* setup runtime args */
	ctx->runtime.jobs = jobs;
	ctx->runtime.done = 0;
	ctx->runtime.proc = proc;
	ctx->runtime.cookie = cookie;

	/* broadcast slices signal about job exist */
	pthread_mutex_lock( &ctx->cond_mutex);
	ctx->readys = 0;
	pthread_cond_broadcast( &ctx->cond_var_job );
	pthread_mutex_unlock( &ctx->cond_mutex);

	/* wait slices threads finished */
	pthread_mutex_lock( &ctx->cond_mutex);
	while ( ctx->readys != ctx->count )
		pthread_cond_wait( &ctx->cond_var_ready, &ctx->cond_mutex );
	pthread_mutex_unlock( &ctx->cond_mutex);
}
