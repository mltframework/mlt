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
#include "mlt_properties.h"
#include "mlt_log.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#ifdef _WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0601
#include <windows.h>
#endif
#define MAX_SLICES 32
#define ENV_SLICES "MLT_SLICES_COUNT"

static pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;
static mlt_properties pool_map = NULL;

struct mlt_slices_runtime_s
{
	int jobs, done, curr;
	mlt_slices_proc proc;
	void* cookie;
	struct mlt_slices_runtime_s* next;
};

struct mlt_slices_s
{
	int f_exit;
	int count;
	int readys;
	int ref;
	pthread_mutex_t cond_mutex;
	pthread_cond_t cond_var_job;
	pthread_cond_t cond_var_ready;
	pthread_t threads[MAX_SLICES];
	struct mlt_slices_runtime_s *head, *tail;
	const char* name;
};

static void* mlt_slices_worker( void* p )
{
	int id, idx;
	struct mlt_slices_runtime_s* r;
	mlt_slices ctx = (mlt_slices)p;

//	mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] entering\n", __FUNCTION__, __LINE__ , ctx, ctx->name );

	pthread_mutex_lock( &ctx->cond_mutex );

	id = ctx->readys;
	ctx->readys++;

	while ( 1 )
	{
//		mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] waiting\n", __FUNCTION__, __LINE__ , ctx, ctx->name );

		/* wait for new jobs */
		while( !ctx->f_exit && !( r = ctx->head ) )
			pthread_cond_wait( &ctx->cond_var_job, &ctx->cond_mutex );

		if ( ctx->f_exit )
			break;

		if ( !r )
			continue;

		/* check if no new job */
		if ( r->curr == r->jobs )
		{
			ctx->head = ctx->head->next;
			if ( !ctx->head )
				ctx->tail = NULL;
//			mlt_log_error( NULL, "%s:%d: new ctx->head=%p\n", __FUNCTION__, __LINE__, ctx->head );
			continue;
		};

		/* new job id */
		idx = r->curr;
		r->curr++;

		/* run job */
		pthread_mutex_unlock( &ctx->cond_mutex );
//		mlt_log_error( NULL, "%s:%d: running job: id=%d, idx=%d/%d, pool=[%s]\n", __FUNCTION__, __LINE__,
//			id, idx, r->jobs, ctx->name );
		r->proc( id, idx, r->jobs, r->cookie );
		pthread_mutex_lock( &ctx->cond_mutex );

		/* increase done jobs counter */
		r->done++;

		/* notify we fininished last job */
		if ( r->done == r->jobs )
		{
//			mlt_log_error( NULL, "%s:%d: pthread_cond_signal( &ctx->cond_var_ready )\n", __FUNCTION__, __LINE__ );
			pthread_cond_broadcast( &ctx->cond_var_ready );
		}
	}

	pthread_mutex_unlock( &ctx->cond_mutex );

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
	int cpus = GetActiveProcessorCount( ALL_PROCESSOR_GROUPS );
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

	pthread_mutex_lock( &pool_lock );

//	mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] closing\n", __FUNCTION__, __LINE__, ctx, ctx->name );

	/* check reference count */
	if ( ctx->ref )
	{
		ctx->ref--;
//		mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] new ref=%d\n", __FUNCTION__, __LINE__, ctx, ctx->name, ctx->ref );
		pthread_mutex_unlock( &pool_lock );
		return;
	}

	/* remove it from pool */
	if ( ctx->name )
	{
		mlt_properties_set_data( pool_map, ctx->name, NULL, 0, NULL, NULL );
//		mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] removed\n", __FUNCTION__, __LINE__, ctx, ctx->name );
	}

	pthread_mutex_unlock( &pool_lock );

	/* notify to exit */
	ctx->f_exit = 1;
	pthread_mutex_lock( &ctx->cond_mutex );
	pthread_cond_broadcast( &ctx->cond_var_job);
	pthread_cond_broadcast( &ctx->cond_var_ready);
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
	struct mlt_slices_runtime_s runtime, *r = &runtime;

//	mlt_log_error( NULL, "%s:%d: entering\n", __FUNCTION__, __LINE__ );

	/* lock */
	pthread_mutex_lock( &ctx->cond_mutex);

	/* check jobs count */
	if ( jobs < 0 )
		jobs = (-jobs) * ctx->count;
	if ( !jobs )
		jobs = ctx->count;

	/* setup runtime args */
	r->jobs = jobs;
	r->done = 0;
	r->curr = 0;
	r->proc = proc;
	r->cookie = cookie;
	r->next = NULL;

	/* attach job */
	if ( ctx->tail )
	{
		ctx->tail->next = r;
		ctx->tail = r;
	}
	else
	{
		ctx->head = ctx->tail = r;
	}

	/* notify workers */
	pthread_cond_broadcast( &ctx->cond_var_job );

//	mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] starting [%d] jobs\n", __FUNCTION__, __LINE__,
//		ctx, ctx->name, r->jobs );

	/* wait for end of task */
	while( !ctx->f_exit && ( r->done < r->jobs ) )
	{
		pthread_cond_wait( &ctx->cond_var_ready, &ctx->cond_mutex );
//		mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] signalled\n", __FUNCTION__, __LINE__ , ctx, ctx->name );
	}

//	mlt_log_error( NULL, "%s:%d: ctx=[%p][%s] finished\n", __FUNCTION__, __LINE__ , ctx, ctx->name );

	pthread_mutex_unlock( &ctx->cond_mutex);
}

/** Initialize a sliced threading context pool
 *
 * \public \memberof mlt_slices_s
 * \param threads number of threads to use for job list
 * \param policy scheduling policy of processing threads
 * \param priority priority value that can be used with the scheduling algorithm
 * \param name name of pool of threads
 * \return the context pointer
 */

mlt_slices mlt_slices_init_pool( int threads, int policy, int priority, const char* name )
{
	mlt_slices ctx = NULL;

	pthread_mutex_lock( &pool_lock );

	/* try to find it by name */
	if ( name )
	{
		if ( !pool_map )
		{
			pool_map = mlt_properties_new();
			mlt_log_error( NULL, "%s:%d: pool_map=%p\n", __FUNCTION__, __LINE__, pool_map );
		}
		else
			ctx = (mlt_slices)mlt_properties_get_data( pool_map, name, 0 );
	}

	if ( !ctx )
	{
		ctx = mlt_slices_init( threads, policy, priority );
		if ( name )
		{
			ctx->name = name;
			mlt_properties_set_data( pool_map, name, ctx, 0, NULL, NULL );
		}
		mlt_log_error( NULL, "%s:%d: initialized pool=[%s]\n", __FUNCTION__, __LINE__, name );
	}
	else
	{
		ctx->ref++;
		mlt_log_error( NULL, "%s:%d: reusing pool=[%s]\n", __FUNCTION__, __LINE__, name );
	}

	pthread_mutex_unlock( &pool_lock );

	return ctx;
}

/** Get the number of slices.
 *
 * \public \memberof mlt_slices_s
 * \param ctx context pointer
 * \return the number of slices
 */

int mlt_slices_count(mlt_slices ctx)
{
	return ctx->count;
}
