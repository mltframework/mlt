/*
 * Copyright (C) 2014 Dan Dennedy <dan@dennedy.org>
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

#ifndef COMMON_H
#define COMMON_H

#include <framework/mlt.h>

class QImage;

bool createQApplicationIfNeeded(mlt_service service);
void convert_qimage_to_mlt_rgba( QImage* qImg, uint8_t* mImg, int width, int height );
void convert_mlt_to_qimage_rgba( uint8_t* mImg, QImage* qImg, int width, int height );
int create_image( mlt_frame frame, uint8_t **image, mlt_image_format *image_format, int *width, int *height, int writable );

#endif // COMMON_H
