/*
 * consumer_qglsl.cpp
 * Copyright (C) 2012-2021 Meltytech, LLC
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

#include "common.h"
#include <framework/mlt.h>
#include <QApplication>
#include <QtGlobal>

#include <QThread>
#include <QOpenGLContext>
#include <QOffscreenSurface>

typedef void* ( *thread_function_t )( void* );

class RenderThread : public QThread
{
public:
	RenderThread(thread_function_t function, void *data)
		: QThread(0)
		, m_function(function)
		, m_data(data)
	{
		m_context = new QOpenGLContext;
		m_context->create();
		m_context->moveToThread(this);
		m_surface = new QOffscreenSurface();
		m_surface->create();
	}
	~RenderThread()
	{
		m_surface->destroy();
		delete m_surface;
	}

protected:
	void run()
	{
		Q_ASSERT(m_context->isValid());
		m_context->makeCurrent(m_surface);
		m_function(m_data);
		m_context->doneCurrent();
		delete m_context;
	}

private:
	thread_function_t m_function;
	void* m_data;
	QOpenGLContext* m_context;
	QOffscreenSurface* m_surface;
};

static void onThreadCreate(mlt_properties owner, mlt_consumer self, mlt_event_data event_data )
{
	Q_UNUSED(owner)
	mlt_event_data_thread* t = (mlt_event_data_thread*) mlt_event_data_to_object(event_data);
	auto thread = new RenderThread((thread_function_t) t->function, t->data);
	*t->thread = thread;
	thread->start();
}

static void onThreadJoin(mlt_properties owner, mlt_consumer self, mlt_event_data event_data)
{
	Q_UNUSED(owner)
	Q_UNUSED(self)
	auto threadData = (mlt_event_data_thread *) mlt_event_data_to_object(event_data);
	if (threadData && threadData->thread) {
		auto thread = (RenderThread *) *threadData->thread;
		if (thread) {
			thread->quit();
			thread->wait();
			qApp->processEvents();
			delete thread;
		}
	}
}

static void onThreadStarted(mlt_properties owner, mlt_consumer consumer)
{
	mlt_service service = MLT_CONSUMER_SERVICE(consumer);
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

	mlt_log_debug(service, "%s\n", __FUNCTION__);
	{
		mlt_events_fire(filter_properties, "init glsl", mlt_event_data_none());
		if (!mlt_properties_get_int(filter_properties, "glsl_supported")) {
			mlt_log_fatal(service,
				"OpenGL Shading Language rendering is not supported on this machine.\n" );
			mlt_events_fire(properties, "consumer-fatal-error", mlt_event_data_none());
		}
	}
}

static void onThreadStopped(mlt_properties owner, mlt_consumer consumer)
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_events_fire(MLT_FILTER_PROPERTIES(filter), "close glsl", mlt_event_data_none());
}

extern "C" {

mlt_consumer consumer_qglsl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_consumer consumer = mlt_factory_consumer(profile, "multi", arg);
	if (consumer) {
		mlt_filter filter = mlt_factory_filter(profile, "glsl.manager", 0);
		if (filter) {
			mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
			mlt_properties_set_data(properties, "glslManager", filter, 0, (mlt_destructor) mlt_filter_close, NULL);
			mlt_events_register( properties, "consumer-cleanup" );
			mlt_events_listen(properties, consumer, "consumer-thread-started", (mlt_listener) onThreadStarted);
			mlt_events_listen(properties, consumer, "consumer-thread-stopped", (mlt_listener) onThreadStopped);
			if (!createQApplicationIfNeeded(MLT_CONSUMER_SERVICE(consumer))) {
				mlt_filter_close(filter);
				mlt_consumer_close(consumer);
				return NULL;
			}
			mlt_events_listen(properties, consumer, "consumer-thread-create", (mlt_listener) onThreadCreate);
			mlt_events_listen(properties, consumer, "consumer-thread-join", (mlt_listener) onThreadJoin);
			qApp->processEvents();
			return consumer;
		}
		mlt_consumer_close(consumer);
	}
	return NULL;
}

}
