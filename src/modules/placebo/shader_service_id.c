/*
 * shader_service_id.c -- service id for an installed placebo shader
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

#include "shader_service_id.h"

#include <string.h>

int shader_service_id(char *id, size_t size, const char *root, const char *path)
{
    const char service[] = "placebo.";
    size_t n = sizeof(service) - 1;
    size_t root_len;
    size_t path_len;
    size_t rel_len;
    const char *rel;

    if (!id || size == 0)
        return -1;
    id[0] = '\0';
    if (!root || !path)
        return -1;
    root_len = strlen(root);
    path_len = strlen(path);
    /* Not root + "/...": do not turn an absolute path into a service id. */
    if (root_len == 0 || path_len <= root_len || strncmp(path, root, root_len) != 0
        || path[root_len] != '/')
        return -1;
    rel = path + root_len + 1;
    rel_len = strlen(rel);
    if (rel_len <= 4 || strcmp(rel + rel_len - 4, ".yml") != 0)
        return -1;
    if (n + rel_len - 4 >= size)
        return -1;
    memcpy(id, service, n);
    for (size_t i = 0; i < rel_len - 4; i++)
        id[n++] = (rel[i] == '/') ? '.' : rel[i];
    id[n] = '\0';
    return 0;
}
