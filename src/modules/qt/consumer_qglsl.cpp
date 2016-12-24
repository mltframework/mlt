/*
 * consumer_qglsl.cpp
 * Copyright (C) 2012-2014 Dan Dennedy <dan@dennedy.org>
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

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)

#include <QGLWidget>
#include <QMutex>
#include <QWaitCondition>

class GLWidget : public QGLWidget
{
private:
	QGLWidget *renderContext;
	bool isInitialized;
	QMutex mutex;
	QWaitCondition condition;

public:
	GLWidget()
#ifdef Q_OS_MAC
		: QGLWidget()
#else
		: QGLWidget(0, 0, Qt::SplashScreen)
#endif
		, renderContext(0)
		, isInitialized(false)
	{
		resize(0, 0);
		show();
	}

	~GLWidget()
	{
		delete renderContext;
	}

	bool createRenderContext()
	{
		if (!isInitialized) {
			mutex.lock();
			condition.wait(&mutex);
			mutex.unlock();
		}
		if (!renderContext) {
			renderContext = new QGLWidget(0, this, Qt::SplashScreen);
			renderContext->resize(0, 0);
			renderContext->makeCurrent();
		}
		return renderContext->isValid();
	}

protected:
	void initializeGL()
	{
		condition.wakeAll();
		isInitialized = true;
	}
};

#else // Qt 5

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

static void onThreadCreate(mlt_properties owner, mlt_consumer self,
	RenderThread** thread, int* priority, thread_function_t function, void* data )
{
	Q_UNUSED(owner)
	Q_UNUSED(priority)
	(*thread) = new RenderThread(function, data);
	(*thread)->start();
}

static void onThreadJoin(mlt_properties owner, mlt_consumer self, RenderThread* thread)
{
	Q_UNUSED(owner)
	Q_UNUSED(self)
	if (thread) {
		thread->quit();
		thread->wait();
		qApp->processEvents();
		delete thread;
	}
}

#endif // Qt 5

static void onThreadStarted(mlt_properties owner, mlt_consumer consumer)
{
	mlt_service service = MLT_CONSUMER_SERVICE(consumer);
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

	mlt_log_debug(service, "%s\n", __FUNCTION__);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	GLWidget *widget = (GLWidget*) mlt_properties_get_data(properties, "GLWidget", NULL);
	if (widget->createRenderContext()) {
#else
	{
#endif
		mlt_events_fire(filter_properties, "init glsl", NULL);
		if (!mlt_properties_get_int(filter_properties, "glsl_supported")) {
			mlt_log_fatal(service,
				"OpenGL Shading Language rendering is not supported on this machine.\n" );
			mlt_events_fire(properties, "consumer-fatal-error", NULL);
		}
	}
}

static void onThreadStopped(mlt_properties owner, mlt_consumer consumer)
{
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_events_fire(MLT_FILTER_PROPERTIES(filter), "close glsl", NULL);
}

static void onCleanup(mlt_properties owner, mlt_consumer consumer)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	GLWidget* widget = (GLWidget*) mlt_properties_get_data( MLT_CONSUMER_PROPERTIES(consumer), "GLWidget", NULL);
	delete widget;
	mlt_properties_set_data(MLT_CONSUMER_PROPERTIES(consumer), "GLWidget", NULL, 0, NULL, NULL);
	qApp->processEvents();
#endif
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
			mlt_events_register( properties, "consumer-cleanup", NULL );
			mlt_events_listen(properties, consumer, "consumer-thread-started", (mlt_listener) onThreadStarted);
			mlt_events_listen(properties, consumer, "consumer-thread-stopped", (mlt_listener) onThreadStopped);
			mlt_events_listen(properties, consumer, "consumer-cleanup", (mlt_listener) onCleanup);
			if (!createQApplicationIfNeeded(MLT_CONSUMER_SERVICE(consumer))) {
				mlt_filter_close(filter);
				mlt_consumer_close(consumer);
				return NULL;
			}
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
			mlt_properties_set_data(properties, "GLWidget", new GLWidget, 0, NULL, NULL);
#else
			mlt_events_listen(properties, consumer, "consumer-thread-create", (mlt_listener) onThreadCreate);
			mlt_events_listen(properties, consumer, "consumer-thread-join", (mlt_listener) onThreadJoin);
#endif
			qApp->processEvents();
			return consumer;
		}
		mlt_consumer_close(consumer);
	}
	return NULL;
}

}
