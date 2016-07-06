/*
 * filter_oldfilm.c -- oldfilm filter
 * Copyright (c) 2007 Marco Gittler <g.marco@freenet.de>
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

#include <framework/mlt_filter.h>
#include <framework/mlt_frame.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

static double sinarr[] = {
0.0,0.0627587292804297,0.125270029508395,0.18728744713136,0.2485664757507,0.308865520098932,
0.3679468485397,0.425577530335206,0.481530353985902,0.535584723021826,0.587527525713892,0.637153975276265,
0.684268417247276,0.728685100865749,0.770228911401552,0.80873606055313,0.84405473219009,0.876045680894979,
0.904582780944473,0.929553523565587,0.95085946050647,0.968416592172968,0.982155698800724,0.99202261335714,
0.997978435097294,0.999999682931835,0.99807838800221,0.992222125098244,0.982453982794196,0.968812472421035,
0.951351376233828,0.930139535372831,0.90526057845426,0.876812591860795,0.844907733031696,0.809671788277164,
0.771243676860277,0.72977490330168,0.685428960066342,0.638380682987321,0.588815561967795,0.536929009678953,
0.48292559113694,0.427018217196276,0.369427305139443,0.310379909672042,0.250108827749629,0.188851680765468,
0.12684997771773,0.064348163049637,0.00159265291648683,-0.0611691363208864,-0.123689763546002,-0.18572273843423,
-0.247023493251739,-0.307350347074556,-0.366465458626247,-0.424135763977612,-0.48013389541149,-0.534239077829989,
-0.586237999170027,-0.635925651395529,-0.683106138750633,-0.727593450087328,-0.769212192222595,-0.807798281433749,
-0.84319959036574,-0.875276547799941,-0.903902688919827,-0.928965153904073,-0.950365132881376,-0.968018255492714,
-0.981854923525203,-0.991820585306115,-0.997875950775248,-0.999997146387718,-0.998175809236459,-0.992419120023356,
-0.982749774749007,-0.969205895232745,-0.951840878815686,-0.930723187839362,-0.905936079729926,-0.877577278752084,
-0.845758590726883,-0.810605462232336,-0.772256486024771,-0.730862854630786,-0.68658776426406,-0.639605771417098,
-0.590102104664575,-0.538271934391528,-0.484319603325524,-0.428457820906457,-0.37090682467023,-0.311893511952568,
-0.251650545336281,-0.190415435368805,-0.128429604166398,-0.0659374335968388,0.0,
};

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position pos = mlt_filter_get_position( filter, frame );
	mlt_position len = mlt_filter_get_length2( filter, frame );

	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	if (  error == 0 && *image )
	{
		int h = *height;
		int w = *width;

		int x = 0;
		int y = 0;

		double position = mlt_filter_get_progress( filter, frame );
		srand( position * 10000);

		int delta = mlt_properties_anim_get_int( properties, "delta", pos, len );
		int every = mlt_properties_anim_get_int( properties, "every", pos, len );

		int bdu = mlt_properties_anim_get_int( properties, "brightnessdelta_up", pos, len );
		int bdd = mlt_properties_anim_get_int( properties, "brightnessdelta_down", pos, len );
		int bevery = mlt_properties_anim_get_int( properties, "brightnessdelta_every", pos, len );

		int udu = mlt_properties_anim_get_int( properties, "unevendevelop_up", pos, len );
		int udd = mlt_properties_anim_get_int( properties, "unevendevelop_down", pos, len );
		int uduration = mlt_properties_anim_get_int( properties, "unevendevelop_duration", pos, len );

		int diffpic = 0;
		if ( delta )
			diffpic = rand() % delta * 2 - delta;
		int brightdelta = 0;
		if (( bdu + bdd ) != 0 )
			brightdelta = rand() % (bdu + bdd) - bdd;
		if ( rand() % 100 > every )
			diffpic = 0;
		if ( rand() % 100 > bevery)
			brightdelta = 0;
		int yend, ydiff;
		int unevendevelop_delta = 0;
		if ( uduration > 0 )
		{
			float uval = sinarr[ ( ((int)position) % uduration) * 100 / uduration ];
			unevendevelop_delta = uval * ( uval > 0 ? udu : udd );
		}
		if ( diffpic <= 0 )
		{
			y = h;
			yend = 0;
			ydiff = -1;
		}
		else
		{
			y = 0;
			yend = h;
			ydiff = 1;
		}

		while( y != yend )
		{
			for ( x = 0; x < w; x++ )
			{
				uint8_t* pic = ( *image + y * w * 2 + x * 2 );
				int newy = y + diffpic;
				if ( newy > 0 && newy < h )
				{
					uint8_t oldval= *( pic + diffpic * w * 2 );
					if ( ((int) oldval + brightdelta + unevendevelop_delta ) > 255 )
					{
						*pic=255;
					}
					else if ( ( (int) oldval + brightdelta + unevendevelop_delta )	<0 )
					{
						*pic=0;
					}
					else
					{
						*pic = oldval + brightdelta + unevendevelop_delta;
					}
					*( pic + 1 ) =* ( pic + diffpic * w * 2 + 1 );

				}
				else
				{
					*pic = 0;
				}
		}
			y += ydiff;
		}
	}

	return error;
}

static mlt_frame filter_process( mlt_filter filter, mlt_frame frame )
{
	mlt_frame_push_service( frame, filter );
	mlt_frame_push_get_image( frame, filter_get_image );
	return frame;
}

mlt_filter filter_oldfilm_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "delta", "14" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "every", "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_up" , "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_down" , "30" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "brightnessdelta_every" , "70" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "unevendevelop_up" , "60" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "unevendevelop_down" , "20" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "unevendevelop_duration" , "70" );
	}
	return filter;
}

