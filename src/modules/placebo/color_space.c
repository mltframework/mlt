/*
 * color_space.c -- map MLT color metadata onto libplacebo
 * Copyright (C) 2026 D-Ogi
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

#include "color_space.h"

#include <framework/mlt_image.h>

/* Match libplacebo's pl_transfer_from_av, not the MLT name. bt709 and
   smpte170m are both the BT.1886 EOTF there. */
enum pl_color_transfer placebo_transfer_from_mlt(mlt_color_trc trc)
{
    switch (trc) {
    case mlt_color_trc_bt709:
    case mlt_color_trc_smpte170m:
    case mlt_color_trc_smpte240m:
    case mlt_color_trc_iec61966_2_4:
    case mlt_color_trc_bt1361_ecg:
    case mlt_color_trc_bt2020_10:
    case mlt_color_trc_bt2020_12:
        return PL_COLOR_TRC_BT_1886;
    case mlt_color_trc_gamma22:
        return PL_COLOR_TRC_GAMMA22;
    case mlt_color_trc_gamma28:
        return PL_COLOR_TRC_GAMMA28;
    case mlt_color_trc_linear:
        return PL_COLOR_TRC_LINEAR;
    case mlt_color_trc_iec61966_2_1:
        return PL_COLOR_TRC_SRGB;
    case mlt_color_trc_smpte2084:
        return PL_COLOR_TRC_PQ;
    case mlt_color_trc_smpte428:
        return PL_COLOR_TRC_ST428;
    case mlt_color_trc_arib_std_b67:
        return PL_COLOR_TRC_HLG;
    case mlt_color_trc_none:
    case mlt_color_trc_unspecified:
    case mlt_color_trc_reserved:
    case mlt_color_trc_log:
    case mlt_color_trc_log_sqrt:
    case mlt_color_trc_invalid:
        break;
    }
    return PL_COLOR_TRC_UNKNOWN;
}

static enum pl_color_primaries primaries_from_mlt(mlt_color_primaries primaries)
{
    switch (primaries) {
    case mlt_color_pri_bt709:
        return PL_COLOR_PRIM_BT_709;
    case mlt_color_pri_bt470m:
        return PL_COLOR_PRIM_BT_470M;
    case mlt_color_pri_bt470bg:
        return PL_COLOR_PRIM_BT_601_625;
    case mlt_color_pri_smpte170m:
        return PL_COLOR_PRIM_BT_601_525;
    case mlt_color_pri_film:
        return PL_COLOR_PRIM_FILM_C;
    case mlt_color_pri_bt2020:
        return PL_COLOR_PRIM_BT_2020;
    case mlt_color_pri_smpte428:
        return PL_COLOR_PRIM_CIE_1931;
    case mlt_color_pri_smpte431:
        return PL_COLOR_PRIM_DCI_P3;
    case mlt_color_pri_smpte432:
        return PL_COLOR_PRIM_DISPLAY_P3;
    case mlt_color_pri_none:
    case mlt_color_pri_invalid:
        break;
    }
    return PL_COLOR_PRIM_UNKNOWN;
}

int placebo_frame_is_linear(mlt_frame frame)
{
    if (!frame)
        return 0;
    const char *name = mlt_properties_get(MLT_FRAME_PROPERTIES(frame), "color_trc");
    return mlt_image_color_trc_id(name) == mlt_color_trc_linear;
}

struct pl_color_space placebo_color_space_for_trc(mlt_frame frame, enum pl_color_transfer transfer)
{
    struct pl_color_space color = {0};
    mlt_properties properties = MLT_FRAME_PROPERTIES(frame);
    const char *primaries_name = mlt_properties_get(properties, "color_primaries");
    mlt_color_primaries primaries = mlt_image_color_pri_id(primaries_name);
    if (primaries == mlt_color_pri_none || primaries == mlt_color_pri_invalid) {
        mlt_colorspace colorspace = mlt_image_colorspace_id(
            mlt_properties_get(properties, "colorspace"));
        int height = mlt_properties_get_int(properties, "height");
        primaries = mlt_image_default_primaries(colorspace, height);
    }
    color.primaries = primaries_from_mlt(primaries);
    color.transfer = transfer;
    return color;
}

pl_fmt placebo_linear_format(pl_gpu gpu)
{
    pl_fmt fmt = gpu ? pl_find_named_fmt(gpu, "rgba16hf") : NULL;
    if (!fmt && gpu)
        fmt = pl_find_named_fmt(gpu, "rgba16f");
    return fmt;
}

pl_fmt placebo_rgba8_format(pl_gpu gpu)
{
    return gpu ? pl_find_named_fmt(gpu, "rgba8") : NULL;
}

void placebo_tex_encoding(pl_gpu gpu, mlt_frame frame, struct pl_color_space *color, pl_fmt *fmt)
{
    pl_fmt rgba8 = placebo_rgba8_format(gpu);
    if (placebo_frame_is_linear(frame)) {
        pl_fmt linear = placebo_linear_format(gpu);
        *color = placebo_color_space_for_trc(frame, PL_COLOR_TRC_LINEAR);
        *fmt = linear ? linear : rgba8;
        return;
    }
    *color = pl_color_space_srgb;
    *fmt = rgba8;
}
