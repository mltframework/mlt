/*
 * filter_dust.c -- dust filter
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

#include <framework/mlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

static void overlay_image(uint8_t *src, int src_width, int src_height , uint8_t *overlay, int overlay_width, int overlay_height, uint8_t * alpha , int xpos, int ypos, int upsidedown , int mirror )
{
	int x,y;

	for ( y = ypos; y < src_height; y++)
	{
		if ( y >= 0 && (y - ypos) < overlay_height )
		{
			uint8_t *scanline_image = src + src_width * y * 2;
			int overlay_y = upsidedown ?  ( overlay_height - ( y - ypos ) - 1 ) : ( y - ypos  );
			uint8_t *scanline_overlay=overlay + overlay_width * 2 * overlay_y;
			
			for ( x = xpos  ;  x < src_width  && x - xpos < overlay_width ;x++ )
			{
				if ( x > 0  )
				{
					int overlay_x = mirror ? overlay_width - ( x - xpos ) -1 : ( x - xpos );
					double alp = (double) * (alpha + overlay_width * overlay_y + overlay_x ) / 255.0;
					uint8_t* image_pixel = scanline_image + x * 2;
					uint8_t* overlay_pixel = scanline_overlay + overlay_x * 2;
					
					*image_pixel = (double)(*overlay_pixel) * alp + (double) *image_pixel * (1.0-alp) ;
					if ( xpos % 2 == 0 )
						image_pixel++;
					else
						image_pixel += 3;

					mirror ? overlay_pixel-- : overlay_pixel++;

					*image_pixel = (double) (*(overlay_pixel)) * alp + (double)(*image_pixel) * ( 1.0 - alp );
				}
		
			}
		}
	}
}

static int filter_get_image( mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable )
{
	mlt_filter filter = (mlt_filter) mlt_frame_pop_service( frame );
	mlt_properties properties = MLT_FILTER_PROPERTIES( filter );
	mlt_position pos = mlt_filter_get_position( filter, frame );
	mlt_position len = mlt_filter_get_length2( filter, frame );
	
	int maxdia = mlt_properties_anim_get_int( properties, "maxdiameter", pos, len );
	int maxcount = mlt_properties_anim_get_int( properties, "maxcount", pos, len );

	*format = mlt_image_yuv422;
	int error = mlt_frame_get_image( frame, image, format, width, height, 1 );

	// Load svg
	char *factory = mlt_properties_get( properties, "factory" );
	char temp[1204] = "";
	sprintf( temp, "%s/oldfilm/", mlt_environment( "MLT_DATA" ) );
	
	mlt_properties direntries = mlt_properties_new();
	mlt_properties_dir_list( direntries, temp,"dust*.svg",1 );
	
	if (!maxcount)
		return 0;

	double position = mlt_filter_get_progress( filter, frame );
	srand( position * 10000 );

	mlt_service_lock( MLT_FILTER_SERVICE( filter ) );

	int im = rand() % maxcount;
	int piccount = mlt_properties_count( direntries );
	while ( im-- && piccount )
	{
		int picnum = rand() % piccount;
		
		int y1 = rand() % *height;
		int x1 = rand() % *width;
		char resource[1024] = "";
		char savename[1024] = "", savename1[1024] = "", cachedy[100];
		int dx = ( *width * maxdia / 100);
		int luma_width, luma_height;
		uint8_t *luma_image = NULL;
		uint8_t *alpha = NULL;
		int updown = rand() % 2;
		int mirror = rand() % 2;
		
		sprintf( resource, "%s", mlt_properties_get_value(direntries,picnum) );
		sprintf( savename, "cache-%d-%d", picnum,dx );
		sprintf( savename1, "cache-alpha-%d-%d", picnum, dx );
		sprintf( cachedy, "cache-dy-%d-%d", picnum,dx );
		
		luma_image = mlt_properties_get_data( properties , savename , NULL );
		alpha = mlt_properties_get_data( properties , savename1 , NULL );
		
		if ( luma_image == NULL || alpha == NULL )
		{
			mlt_profile profile = mlt_service_profile( MLT_FILTER_SERVICE( filter ) );
			mlt_producer producer = mlt_factory_producer( profile, factory, resource );
		
			if ( producer != NULL )
			{
				mlt_properties producer_properties = MLT_PRODUCER_PROPERTIES( producer );
				
				mlt_properties_set( producer_properties, "eof", "loop" );
				mlt_frame luma_frame = NULL;
				
				if ( mlt_service_get_frame( MLT_PRODUCER_SERVICE( producer ), &luma_frame, 0 ) == 0 )
				{
					mlt_image_format luma_format = mlt_image_yuv422;
					luma_width = dx;
					luma_height = luma_width * mlt_properties_get_int( MLT_FRAME_PROPERTIES ( luma_frame ) , "height" ) / mlt_properties_get_int( MLT_FRAME_PROPERTIES ( luma_frame ) , "width" );

					mlt_properties_set( MLT_FRAME_PROPERTIES( luma_frame ), "rescale.interp", "best" );// none/nearest/tiles/hyper
					
					mlt_frame_get_image( luma_frame, &luma_image, &luma_format, &luma_width, &luma_height, 0 );
					alpha = mlt_frame_get_alpha_mask (luma_frame );
					
					uint8_t* savealpha = mlt_pool_alloc( luma_width * luma_height );
					uint8_t* savepic = mlt_pool_alloc( luma_width * luma_height * 2);
					
					if ( savealpha && savepic )
					{
						memcpy( savealpha, alpha , luma_width * luma_height );
						memcpy( savepic, luma_image , luma_width * luma_height * 2 );
						
						mlt_properties_set_data( properties, savename, savepic, luma_width * luma_height * 2, mlt_pool_release, NULL );
						mlt_properties_set_data( properties, savename1, savealpha, luma_width * luma_height,  mlt_pool_release, NULL );
						mlt_properties_set_int( properties, cachedy, luma_height );
						
						overlay_image( *image, *width, *height, luma_image, luma_width, luma_height, alpha, x1, y1, updown, mirror );
					}
					else
					{
						if ( savealpha )
							mlt_pool_release( savealpha );
						if ( savepic )
							mlt_pool_release( savepic );
					}
					mlt_frame_close( luma_frame );	
				}
				mlt_producer_close( producer );	
			}
		}
		else 
		{
			overlay_image ( *image, *width, *height, luma_image, dx, mlt_properties_get_int ( properties, cachedy ), alpha, x1, y1, updown, mirror );
		}
	}

	mlt_service_unlock( MLT_FILTER_SERVICE( filter ) );

	if (piccount>0 )
		return 0;
	if ( error == 0 && *image )
	{

		int h = *height;
		int w = *width;
		int im = rand() % maxcount;
		
		while ( im-- )
		{
			int type = im % 2;
			int y1 = rand() % h;
			int x1 = rand() % w;
			int dx = rand() % maxdia;
			int dy = rand() % maxdia;
			int x=0, y=0;
			double v = 0.0;
			for ( x = -dx ; x < dx ; x++ )
			{
				for ( y = -dy ; y < dy ; y++ ) 
				{
					if ( x1 + x < w && x1 + x > 0 && y1 + y < h && y1 + y > 0 ){
						uint8_t *pix = *image + (y+y1) * w * 2 + (x + x1) * 2;

						v=pow((double) x /(double)dx * 5.0, 2.0) + pow((double)y / (double)dy * 5.0, 2.0);
						if (v>10)
							v=10;
						v = 1.0 - ( v / 10.0 );

						switch(type)
						{
							case 0:
								*pix -= (*pix) * v;
								break;
							case 1:
								*pix += ( 255-*pix ) * v;
								break;
						}
					}
				}
			}
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

mlt_filter filter_dust_init( mlt_profile profile, mlt_service_type type, const char *id, char *arg )
{
	mlt_filter filter = mlt_filter_new( );
	if ( filter != NULL )
	{
		filter->process = filter_process;
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "maxdiameter", "2" );
		mlt_properties_set( MLT_FILTER_PROPERTIES( filter ), "maxcount", "10" );
	}
	return filter;
}


