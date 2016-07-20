/*
 * factory.c -- the factory method interfaces
 * Copyright (C) 2006 Visual Media
 * Author: Charles Yates <charles.yates@gmail.com>
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

#include <string.h>
#include <limits.h>
#include <framework/mlt.h>

#ifdef USE_QT_OPENGL
extern mlt_consumer consumer_qglsl_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
#endif
extern mlt_filter filter_audiowaveform_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_qimage_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_qtext_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_producer producer_kdenlivetitle_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_transition transition_vqm_init( mlt_profile profile, mlt_service_type type, const char *id, void *arg );
extern mlt_transition transition_qtblend_init( mlt_profile profile, mlt_service_type type, const char *id, void *arg );
extern mlt_filter filter_qtblend_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );

#ifdef USE_FFTW
extern mlt_filter filter_audiospectrum_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
extern mlt_filter filter_lightshow_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg );
#endif

static mlt_properties metadata( mlt_service_type type, const char *id, void *data )
{
	char file[ PATH_MAX ];
	snprintf( file, PATH_MAX, "%s/qt/%s", mlt_environment( "MLT_DATA" ), (char*) data );
	return mlt_properties_parse_yaml( file );
}

MLT_REPOSITORY
{
#ifdef USE_QT_OPENGL
	MLT_REGISTER( consumer_type, "qglsl", consumer_qglsl_init );
#endif
	MLT_REGISTER( filter_type, "audiowaveform", filter_audiowaveform_init );
	MLT_REGISTER( producer_type, "qimage", producer_qimage_init );
	MLT_REGISTER( producer_type, "qtext", producer_qtext_init );
	MLT_REGISTER( producer_type, "kdenlivetitle", producer_kdenlivetitle_init );
	MLT_REGISTER( transition_type, "qtblend", transition_qtblend_init );
	MLT_REGISTER( filter_type, "qtblend", filter_qtblend_init );
	MLT_REGISTER_METADATA( transition_type, "qtblend", metadata, "transition_qtblend.yml" );
	MLT_REGISTER_METADATA( filter_type, "qtblend", metadata, "filter_qtblend.yml" );
#ifdef USE_FFTW
	MLT_REGISTER( filter_type, "audiospectrum", filter_audiospectrum_init );
	MLT_REGISTER( filter_type, "lightshow", filter_lightshow_init );
#endif
	MLT_REGISTER_METADATA( filter_type, "audiowaveform", metadata, "filter_audiowaveform.yml" );
#ifdef USE_FFTW
	MLT_REGISTER_METADATA( filter_type, "lightshow", metadata, "filter_lightshow.yml" );
	MLT_REGISTER_METADATA( filter_type, "audiospectrum", metadata, "filter_audiospectrum.yml" );
#endif
	MLT_REGISTER_METADATA( producer_type, "qimage", metadata, "producer_qimage.yml" );
	MLT_REGISTER_METADATA( producer_type, "qtext", metadata, "producer_qtext.yml" );
	MLT_REGISTER_METADATA( producer_type, "kdenlivetitle", metadata, "producer_kdenlivetitle.yml" );
#ifdef GPL3
	MLT_REGISTER( transition_type, "vqm", transition_vqm_init );
	MLT_REGISTER_METADATA( transition_type, "vqm", metadata, "transition_vqm.yml" );
#endif
}
