/**
 *	file: arrow_code.h
 *
 *	/brief Misc functions to draw arrows
 *	/author Zachary Drew, Copyright 2004
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

extern int init_arrows( mlt_image_format *image_format, int width, int height );
extern void draw_line(uint8_t *buf, int sx, int sy, int ex, int ey, int color);
extern void draw_arrow(uint8_t *buf, int sx, int sy, int ex, int ey, int color);
extern void draw_rectangle_fill(uint8_t *buf, int x, int y, int w, int h, int color);
extern void draw_rectangle_outline(uint8_t *buf, int x, int y, int w, int h, int color);
