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
#include <QtGui/QApplication>
#include <QtCore/QLocale>
#include <QtOpenGL/QGLWidget>

static void onThreadStarted(mlt_properties owner, mlt_consumer consumer)
{
	mlt_service service = MLT_CONSUMER_SERVICE(consumer);
	mlt_properties properties = MLT_CONSUMER_PROPERTIES(consumer);
	mlt_filter filter = (mlt_filter) mlt_properties_get_data(properties, "glslManager", NULL);
	mlt_properties filter_properties = MLT_FILTER_PROPERTIES(filter);
	QApplication* app = qApp;

	mlt_log_debug(service, "%s\n", __FUNCTION__);
#ifdef linux
	if ( getenv("DISPLAY") == 0 ) {
		mlt_log_error(service, "The qglsl consumer requires a X11 environment.\nPlease either run melt from an X session or use a fake X server like xvfb:\nxvfb-run -a melt (...)\n" );
	} else
#endif
	if (!app) {
		int argc = 1;
		char* argv[1];
		argv[0] = (char*) "MLT qglsl consumer";
		app = new QApplication(argc, argv);
		const char *localename = mlt_properties_get_lcnumeric(properties);
		QLocale::setDefault(QLocale(localename));
	}
	QGLWidget* renderContext = new QGLWidget;
	renderContext->resize(0, 0);
	renderContext->show();
	mlt_events_fire(filter_properties, "init glsl", NULL);
	if (!mlt_properties_get_int(filter_properties, "glsl_supported")) {
		mlt_log_fatal(service,
			"OpenGL Shading Language rendering is not supported on this machine.\n" );
		mlt_events_fire(properties, "consumer-fatal-error", NULL);
	}
	else {
		mlt_properties_set_data(properties, "qglslRenderContext", renderContext, 0, NULL, NULL);
	}
}

static void onCleanup(mlt_properties owner, mlt_consumer consumer)
{
	QGLWidget* renderContext = (QGLWidget*) mlt_properties_get_data(
		MLT_CONSUMER_PROPERTIES(consumer), "qglslRenderContext", NULL);
	if (renderContext)
		renderContext->makeCurrent();
	delete renderContext;
	mlt_properties_set_data(MLT_CONSUMER_PROPERTIES(consumer),
		"qglslRenderContext", NULL, 0, NULL, NULL);
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
			mlt_events_listen(properties, consumer, "consumer-cleanup", (mlt_listener) onCleanup);
			return consumer;
		}
		mlt_consumer_close(consumer);
	}
	return NULL;
}

}
