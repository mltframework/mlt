/*
 * common.h
 * Copyright (C) 2013 Jakub Ksiezniak <j.ksiezniak@gmail.com>
 * Copyright (C) 2014 Brian Matherly <pez4brian@yahoo.com>
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

#ifndef VIDSTAB_COMMON_H_
#define VIDSTAB_COMMON_H_

#include <vid.stab/libvidstab.h>
#include <framework/mlt.h>

mlt_image_format validate_format( mlt_image_format format );
VSPixelFormat mltimage_to_vsimage( mlt_image_format mlt_format, int width, int height, uint8_t* mlt_img, uint8_t** vs_img );
void vsimage_to_mltimage( uint8_t* vs_img, uint8_t* mlt_img, mlt_image_format mlt_format, int width, int height );
void free_vsimage( uint8_t* vs_img, VSPixelFormat format );

int compare_motion_config( VSMotionDetectConfig* a, VSMotionDetectConfig* b );
int compare_transform_config( VSTransformConfig* a, VSTransformConfig* b );

void init_vslog();

#endif /* VIDSTAB_COMMON_H_ */
