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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef COMMON_H
#define COMMON_H

#include <framework/mlt.h>

class QImage;

bool createQApplicationIfNeeded(mlt_service service);
void copy_qimage_to_mlt_rgba( QImage* qImg, uint8_t* mImg );
void copy_mlt_to_qimage_rgba( uint8_t* mImg, QImage* qImg );

#endif // COMMON_H
