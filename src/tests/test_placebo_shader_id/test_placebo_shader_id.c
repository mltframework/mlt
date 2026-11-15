/*
 * test_placebo_shader_id.c -- calls the placebo module's shader id rule
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

#include <stdio.h>
#include <string.h>

static int check_id(const char *root, const char *path, const char *want)
{
    char id[256];

    memset(id, 'X', sizeof(id));
    if (shader_service_id(id, sizeof(id), root, path) != 0 || strcmp(id, want) != 0) {
        fprintf(stderr, "FAIL %s -> '%s' want '%s'\n", path, id, want);
        return 1;
    }
    printf("ok %s\n", id);
    return 0;
}

/* The shipped rule must fail and leave id empty, so it cannot be the
   absolute path with '/' replaced by '.'. */
static int check_rejected(const char *root, const char *path)
{
    char id[512];

    memset(id, 'X', sizeof(id) - 1);
    id[sizeof(id) - 1] = '\0';
    if (shader_service_id(id, sizeof(id), root, path) == 0 || id[0] != '\0') {
        fprintf(stderr, "FAIL outside path %s produced '%s'\n", path, id);
        return 1;
    }
    printf("ok rejected %s\n", path);
    return 0;
}

int main(void)
{
    const char *root = "/data/placebo/shaders";
    int errors = 0;

    errors += check_id(root, "/data/placebo/shaders/plain.yml", "placebo.plain");
    errors += check_id(root,
                       "/data/placebo/shaders/garamond13/gaussian_blur.yml",
                       "placebo.garamond13.gaussian_blur");
    errors += check_id(root,
                       "/data/placebo/shaders/garamond13/unsharp_mask.yml",
                       "placebo.garamond13.unsharp_mask");
    errors += check_id(root,
                       "/data/placebo/shaders/a/b/unsharp_mask.yml",
                       "placebo.a.b.unsharp_mask");
    errors += check_rejected(root, "/tmp/not-shaders/gaussian_blur.yml");
    errors += check_rejected(root, "/data/placebo/shaders-extra/gaussian_blur.yml");
    return errors == 0 ? 0 : 1;
}
