/*
 * shader_service_id.h -- service id for an installed placebo shader
 * Copyright (C) 2026 Meltytech, LLC
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

#ifndef SHADER_SERVICE_ID_H
#define SHADER_SERVICE_ID_H

#include <stddef.h>

/* root has no trailing slash.
   "root/gaussian_blur.yml" -> "placebo.gaussian_blur"
   "root/garamond13/gaussian_blur.yml" -> "placebo.garamond13.gaussian_blur"
   A path that is not root + "/" fails and leaves id empty. */
int shader_service_id(char *id, size_t size, const char *root, const char *path);

#endif
