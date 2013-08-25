/*
 * consumer_qglsl.cpp
 * Copyright (C) 2012 Dan Dennedy <dan@dennedy.org>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <framework/mlt.h>
#include <QApplication>
#include <QLocale>
#include <QGLWidget>
#include <QMutex>
#include <QWaitCondition>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
#include <X11/Xlib.h>
#endif

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

static void onThreadStarted(mlt_properties owner, mlt_consumer consumer)
{
	mlt_service service = MLT_CONSUMER_SERVICE(consumer);
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);

	mlt_log_debug(service, "%s\n", __FUNCTION__);
	GLWidget *widget = (GLWidget*) mlt_properties_get_data(properties, "GLWidget", NULL);
	if (widget->createRenderContext()) {
		mlt_events_fire(filter_properties, "init glsl", NULL);
		if (!mlt_properties_get_int(filter_properties, "glsl_supported")) {
			mlt_log_fatal(service,
				"OpenGL Shading Language rendering is not supported on this machine.\n" );
			mlt_events_fire(properties, "consumer-fatal-error", NULL);
		}
	}
}

static void onCleanup(mlt_properties owner, mlt_consumer consumer)
{
	GLWidget* widget = (GLWidget*) mlt_properties_get_data( MLT_CONSUMER_PROPERTIES(consumer), "GLWidget", NULL);
	delete widget;
	mlt_properties_set_data(MLT_CONSUMER_PROPERTIES(consumer), "GLWidget", NULL, 0, NULL, NULL);
	qApp->processEvents();
}

extern "C" {

mlt_consumer consumer_qglsl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_consumer consumer = mlt_factory_consumer(profile, "multi", arg);
	if (consumer) {
		mlt_filter filter = mlt_factory_filter(profile, "glsl.manager", 0);
		if (filter) {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
			XInitThreads();
#endif
			mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
			mlt_properties_set_data(properties, "glslManager", filter, 0, (mlt_destructor) mlt_filter_close, NULL);
			mlt_events_register( properties, "consumer-cleanup", NULL );
			mlt_events_listen(properties, consumer, "consumer-thread-started", (mlt_listener) onThreadStarted);
			mlt_events_listen(properties, consumer, "consumer-cleanup", (mlt_listener) onCleanup);
#ifdef linux
			if ( getenv("DISPLAY") == 0 ) {
				mlt_log_error(MLT_CONSUMER_SERVICE(consumer), "The qglsl consumer requires a X11 environment.\nPlease either run melt from an X session or use a fake X server like xvfb:\nxvfb-run -a melt (...)\n" );
			} else
#endif
			if (!qApp) {
				int argc = 1;
				char* argv[1];
				argv[0] = (char*) "MLT qglsl consumer";
				new QApplication(argc, argv);
				const char *localename = mlt_properties_get_lcnumeric(properties);
				QLocale::setDefault(QLocale(localename));
			}
			mlt_properties_set_data(properties, "GLWidget", new GLWidget, 0, NULL, NULL);
			qApp->processEvents();
			return consumer;
		}
		mlt_consumer_close(consumer);
	}
	return NULL;
}

}
