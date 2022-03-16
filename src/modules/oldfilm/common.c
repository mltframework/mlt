/*
 * Copyright (C) 2022 Meltytech, LLC
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
#include <stdlib.h>


void oldfilm_init_seed( oldfilm_rand_seed* seed, int init )
{
	// Use the initial value to initialize the seed to arbitrary values.
	// This causes the algorithm to produce consistent results each time for the same frame number.
	seed->x = 521288629 + init - ( init << 16 );
	seed->y = 362436069 - init + ( init << 16 );
}

int oldfilm_fast_rand( oldfilm_rand_seed* seed )
{
	static unsigned int a = 18000, b = 30903;
	seed->x = a * ( seed->x & 65535 ) + ( seed->x >> 16 );
	seed->y = b * ( seed->y & 65535 ) + ( seed->y >> 16 );
	int r = ( seed->x << 16 ) + ( seed->y & 65535 );
	return abs(r);
}
