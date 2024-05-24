/*
 * LV2 URID Helpers
 *
 * Adapted from lv2bench.c from lilv project https://drobilla.net/software/lilv.html - http://gitlab.com/lv2/lilv.git
 *
 * Copyright (C) David Robillard 2012-2019 (d@drobilla.net)
 * Copyright (C) mr.fantastic (mrfantastic@firemail.cc) and MLT project 2024
 *
 * ISC License
 * SPDX-License-Identifier: ISC
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef __LV2_URID_HELPER_H__
#define __LV2_URID_HELPER_H__

typedef struct {
  char** uris;
  size_t n_uris;
} URITable;

static void
uri_table_init(URITable* table)
{
  table->uris   = NULL;
  table->n_uris = 0;
}

static LV2_URID
uri_table_map(LV2_URID_Map_Handle handle, const char* uri)
{
  URITable* table = (URITable*)handle;
  for (size_t i = 0; i < table->n_uris; ++i) {
    if (!strcmp(table->uris[i], uri)) {
      return i + 1;
    }
  }

  const size_t len = strlen(uri);
  table->uris = (char**)realloc(table->uris, ++table->n_uris * sizeof(char*));
  table->uris[table->n_uris - 1] = (char*)malloc(len + 1);
  memcpy(table->uris[table->n_uris - 1], uri, len + 1);
  return table->n_uris;
}

static const char*
uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid)
{
  URITable* table = (URITable*)handle;
  if (urid > 0 && urid <= table->n_uris) {
    return table->uris[urid - 1];
  }
  return NULL;
}

#endif /* __LV2_URID_HELPER_H__ */
