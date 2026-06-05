/*
 * mlt_openfx.c -- common mlt functions for OpenFX plugins
 * Copyright (C) 2025-2026 Meltytech, LLC
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "mlt_openfx.h"
#include <framework/mlt_slices.h>

#include <errno.h>
#include <float.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif

/* OpenFX Header files https://github.com/AcademySoftwareFoundation/openfx/tree/main/include */
#include <ofxDrawSuite.h>
#include <ofxGPURender.h>
#include <ofxImageEffect.h>
#include <ofxOpenGLRender.h>
#include <ofxParametricParam.h>
#include <ofxProgress.h>
#include <ofxTimeLine.h>

static inline uint16_t f32_to_f16(float f)
{
    uint32_t x;
    memcpy(&x, &f, sizeof(x));
    uint16_t sign = (x >> 31) & 0x0001U;
    int32_t exp = ((x >> 23) & 0x00FFU) - 127 + 15;
    uint16_t mant = (x >> 13) & 0x03FFU;
    if (exp <= 0)
        return (uint16_t) (sign << 15);
    if (exp >= 31)
        return (uint16_t) ((sign << 15) | 0x7C00U);
    return (uint16_t) ((sign << 15) | ((uint16_t) exp << 10) | mant);
}

static inline float f16_to_f32(uint16_t h)
{
    uint32_t sign = (h >> 15) & 0x0001U;
    uint32_t exp = (h >> 10) & 0x001FU;
    uint32_t mant = h & 0x03FFU;
    uint32_t x;
    if (exp == 0)
        x = (sign << 31) | (mant << 13);
    else if (exp == 31)
        x = (sign << 31) | (0xFFU << 23) | (mant << 13);
    else
        x = (sign << 31) | ((exp - 15 + 127) << 23) | (mant << 13);
    float f;
    memcpy(&f, &x, sizeof(f));
    return f;
}

static int mltofx_source_prefers_top_left_origin(const OfxPlugin *plugin, const char *depth_format)
{
    if (!plugin || !plugin->pluginIdentifier || !depth_format)
        return 0;

    // GMIC's byte-depth path expects top-left memory addressing for Source.
    return !strncmp(plugin->pluginIdentifier, "eu.gmic.", 8)
           && !strcmp(depth_format, kOfxBitDepthByte);
}

uint16_t *mltofx_rgba64_to_half(const uint16_t *src, int n_pixels)
{
    int count = n_pixels * 4;
    uint16_t *dst = malloc((size_t) count * sizeof(uint16_t));
    if (!dst)
        return NULL;
    int i;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (i = 0; i < count; ++i)
        dst[i] = f32_to_f16((float) src[i] * (1.0f / 65535.0f));
    return dst;
}

void mltofx_half_to_rgba64(const uint16_t *src, uint16_t *dst, int n_pixels)
{
    int count = n_pixels * 4;
    int i;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (i = 0; i < count; ++i) {
        float v = f16_to_f32(src[i]);
        // This clamping using >= and <= handles NaN and +/-inf
        v = (v >= 0.0f) ? (v <= 1.0f ? v : 1.0f) : 0.0f;
        dst[i] = (uint16_t) (v * 65535.0f + 0.5f);
    }
}

float *mltofx_rgba64_to_float(const uint16_t *src, int n_pixels)
{
    int count = n_pixels * 4;
    float *dst = malloc((size_t) count * sizeof(float));
    if (!dst)
        return NULL;
    int i;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (i = 0; i < count; ++i)
        dst[i] = (float) src[i] * (1.0f / 65535.0f);
    return dst;
}

void mltofx_float_to_rgba64(const float *src, uint16_t *dst, int n_pixels)
{
    int count = n_pixels * 4;
    int i;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (i = 0; i < count; ++i) {
        float v = src[i];
        // This clamping using >= and <= handles NaN and +/-inf
        v = (v >= 0.0f) ? (v <= 1.0f ? v : 1.0f) : 0.0f;
        dst[i] = (uint16_t) (v * 65535.0f + 0.5f);
    }
}

static OfxStatus propGetInt(OfxPropertySetHandle properties,
                            const char *property,
                            int index,
                            int *value);
static OfxStatus propGetIntN(OfxPropertySetHandle properties,
                             const char *property,
                             int count,
                             int *value);
static OfxStatus propSetString(OfxPropertySetHandle properties,
                               const char *property,
                               int index,
                               const char *value);

static OfxStatus getPropertySet(OfxImageEffectHandle imageEffect, OfxPropertySetHandle *propHandle)
{
    if (!imageEffect)
        return kOfxStatErrBadHandle;
    mlt_properties image_effect = (mlt_properties) imageEffect;
    *propHandle = (OfxPropertySetHandle) mlt_properties_get_properties(image_effect, "props");
    return kOfxStatOK;
}

static OfxStatus getParamSet(OfxImageEffectHandle imageEffect, OfxParamSetHandle *paramSet)
{
    if (!imageEffect)
        return kOfxStatErrBadHandle;
    *paramSet = (OfxParamSetHandle) mlt_properties_get_properties((mlt_properties) imageEffect,
                                                                  "params");
    return kOfxStatOK;
}

static OfxStatus clipDefine(OfxImageEffectHandle imageEffect,
                            const char *name,
                            OfxPropertySetHandle *propertySet)
{
    mlt_log_debug(NULL, "[openfx] clipDefine: `%s` ## %p\n", name, propertySet);
    if (!imageEffect)
        return kOfxStatErrBadHandle;
    mlt_properties set = mlt_properties_get_properties((mlt_properties) imageEffect, "clips");
    mlt_properties clip = mlt_properties_new();
    mlt_properties clip_props = mlt_properties_new();
    if (!clip || !clip_props)
        return kOfxStatErrMemory;
    mlt_properties_set_properties(set, name, clip);
    mlt_properties_close(clip);
    mlt_properties_set_properties(clip, "props", clip_props);
    mlt_properties_close(clip_props);
    *propertySet = (OfxPropertySetHandle) clip_props;

    propSetString((OfxPropertySetHandle) clip_props,
                  kOfxImageEffectPropPreMultiplication,
                  0,
                  kOfxImageUnPreMultiplied);

    return kOfxStatOK;
}

static OfxStatus clipGetHandle(OfxImageEffectHandle imageEffect,
                               const char *name,
                               OfxImageClipHandle *clip,
                               OfxPropertySetHandle *propertySet)
{
    if (!imageEffect)
        return kOfxStatErrBadHandle;
    mlt_properties set = mlt_properties_get_properties((mlt_properties) imageEffect, "clips");
    mlt_properties clip_temp = mlt_properties_get_properties(set, name);
    *clip = (OfxImageClipHandle) clip_temp;
    if (propertySet != NULL)
        *propertySet = (OfxPropertySetHandle) mlt_properties_get_properties(clip_temp, "props");
    return kOfxStatOK;
}

static OfxStatus clipGetPropertySet(OfxImageClipHandle clip, OfxPropertySetHandle *propHandle)
{
    if (!clip)
        return kOfxStatErrBadHandle;
    *propHandle = (OfxPropertySetHandle) mlt_properties_get_properties((mlt_properties) clip,
                                                                       "props");
    return kOfxStatOK;
}

static OfxStatus clipGetImage(OfxImageClipHandle clip,
                              OfxTime time,
                              const OfxRectD *region,
                              OfxPropertySetHandle *imageHandle)
{
    (void) time;
    (void) region;
    if (!clip)
        return kOfxStatErrBadHandle;
    *imageHandle = (OfxPropertySetHandle) mlt_properties_get_properties((mlt_properties) clip,
                                                                        "props");
    return kOfxStatOK;
}

static OfxStatus clipReleaseImage(OfxPropertySetHandle imageHandle)
{
    return kOfxStatOK;
}

static OfxStatus clipGetRegionOfDefinition(OfxImageClipHandle clip, OfxTime time, OfxRectD *bounds)
{
    if (!clip || !bounds) {
        return kOfxStatErrBadHandle;
    }
    mlt_properties clip_prop;
    clipGetPropertySet(clip, (OfxPropertySetHandle *) &clip_prop);

    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    propGetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 0, &x1);
    propGetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 1, &y1);
    propGetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 2, &x2);
    propGetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 3, &y2);
    // OFX spec requires canonical coordinates: x scaled by PAR, y in pixels.
    double par = 1.0;
    mlt_properties par_props = mlt_properties_get_properties(clip_prop, "0");
    if (par_props && mlt_properties_exists(par_props, kOfxImagePropPixelAspectRatio))
        par = mlt_properties_get_double(par_props, kOfxImagePropPixelAspectRatio);
    if (par <= 0.0)
        par = 1.0;
    bounds->x1 = (double) x1 * par;
    bounds->y1 = (double) y1;
    bounds->x2 = (double) x2 * par;
    bounds->y2 = (double) y2;

    if (bounds->x2 < bounds->x1 || bounds->y2 < bounds->y1) {
        // the RoD is invalid (empty is OK)
        return kOfxStatFailed;
    }

    return kOfxStatOK;
}

static int abortOfxFn(OfxImageEffectHandle imageEffect)
{
    return 0;
}

static OfxStatus imageMemoryAlloc(OfxImageEffectHandle instanceHandle,
                                  size_t nBytes,
                                  OfxImageMemoryHandle *memoryHandle)
{
    if (instanceHandle == NULL) {
        *memoryHandle = NULL;
        return kOfxStatErrBadHandle;
    }

    void *temp = calloc(1, nBytes);

    if (temp == NULL)
        return kOfxStatErrMemory;

    *memoryHandle = temp;

    return kOfxStatOK;
}

static OfxStatus imageMemoryFree(OfxImageMemoryHandle memoryHandle)
{
    free(memoryHandle);

    return kOfxStatOK;
}

static OfxStatus imageMemoryLock(OfxImageMemoryHandle memoryHandle, void **returnedPtr)
{
    if (returnedPtr)
        *returnedPtr = memoryHandle;
    return kOfxStatOK;
}

static OfxStatus imageMemoryUnlock(OfxImageMemoryHandle memoryHandle)
{
    return kOfxStatOK;
}

static OfxImageEffectSuiteV1 MltOfxImageEffectSuiteV1 = {getPropertySet,
                                                         getParamSet,
                                                         clipDefine,
                                                         clipGetHandle,
                                                         clipGetPropertySet,
                                                         clipGetImage,
                                                         clipReleaseImage,
                                                         clipGetRegionOfDefinition,
                                                         abortOfxFn,
                                                         imageMemoryAlloc,
                                                         imageMemoryFree,
                                                         imageMemoryLock,
                                                         imageMemoryUnlock};

static mlt_properties fetch_mlt_properties(OfxPropertySetHandle properties, int index)
{
    if (index < 0)
        return NULL;
    mlt_properties props = (mlt_properties) properties;
    char index_str[12] = {'\0'};
    snprintf(index_str, sizeof(index_str), "%d", index);
    mlt_properties p = mlt_properties_get_properties(props, index_str);
    return p;
}

static mlt_properties get_mlt_properties(OfxPropertySetHandle properties, int index)
{
    if (index < 0)
        return NULL;
    mlt_properties props = (mlt_properties) properties;
    char index_str[12] = {'\0'};
    snprintf(index_str, sizeof(index_str), "%d", index);
    mlt_properties p = mlt_properties_get_properties(props, index_str);
    if (!p) {
        p = mlt_properties_new();
        mlt_properties_set_properties(props, index_str, p);
        mlt_properties_close(p);
    }
    return p;
}

static OfxStatus propSetPointer(OfxPropertySetHandle properties,
                                const char *property,
                                int index,
                                void *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = get_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    mlt_properties_set_data(p, property, value, 0, NULL, NULL);
    return kOfxStatOK;
}

static OfxStatus propSetString(OfxPropertySetHandle properties,
                               const char *property,
                               int index,
                               const char *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = get_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    mlt_properties_set(p, property, value);
    return kOfxStatOK;
}

static OfxStatus propSetDouble(OfxPropertySetHandle properties,
                               const char *property,
                               int index,
                               double value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = get_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    mlt_properties_set_double(p, property, value);
    return kOfxStatOK;
}

static OfxStatus propSetInt(OfxPropertySetHandle properties,
                            const char *property,
                            int index,
                            int value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = get_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    mlt_properties_set_int(p, property, value);
    return kOfxStatOK;
}

static OfxStatus propSetPointerN(OfxPropertySetHandle properties,
                                 const char *property,
                                 int count,
                                 void *const *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = get_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        mlt_properties_set_data(p, property, value[i], 0, NULL, NULL);
    }
    return kOfxStatOK;
}

static OfxStatus propSetStringN(OfxPropertySetHandle properties,
                                const char *property,
                                int count,
                                const char *const *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = get_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        mlt_properties_set(p, property, value[i]);
    }
    return kOfxStatOK;
}

static OfxStatus propSetDoubleN(OfxPropertySetHandle properties,
                                const char *property,
                                int count,
                                const double *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = get_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        mlt_properties_set_double(p, property, value[i]);
    }
    return kOfxStatOK;
}

static OfxStatus propSetIntN(OfxPropertySetHandle properties,
                             const char *property,
                             int count,
                             const int *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = get_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        mlt_properties_set_int(p, property, value[i]);
    }
    return kOfxStatOK;
}

static OfxStatus propGetPointer(OfxPropertySetHandle properties,
                                const char *property,
                                int index,
                                void **value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = fetch_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    if (!mlt_properties_exists(p, property)) {
        mlt_log_debug(NULL, "[openfx] propGetPointer: '%s'[%d] not found\n", property, index);
        return kOfxStatErrUnknown;
    }
    *value = mlt_properties_get_data(p, property, NULL);
    return kOfxStatOK;
}

static OfxStatus propGetString(OfxPropertySetHandle properties,
                               const char *property,
                               int index,
                               char **value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = fetch_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    if (!mlt_properties_exists(p, property)) {
        mlt_log_debug(NULL, "[openfx] propGetString: '%s'[%d] not found\n", property, index);
        return kOfxStatErrUnknown;
    }
    *value = mlt_properties_get(p, property);
    return kOfxStatOK;
}

static OfxStatus propGetDouble(OfxPropertySetHandle properties,
                               const char *property,
                               int index,
                               double *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = fetch_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    if (!mlt_properties_exists(p, property)) {
        mlt_log_debug(NULL, "[openfx] propGetDouble: '%s'[%d] not found\n", property, index);
        return kOfxStatErrUnknown;
    }
    *value = mlt_properties_get_double(p, property);
    return kOfxStatOK;
}

static OfxStatus propGetInt(OfxPropertySetHandle properties,
                            const char *property,
                            int index,
                            int *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties p = fetch_mlt_properties(properties, index);
    if (!p)
        return kOfxStatErrBadIndex;
    if (!mlt_properties_exists(p, property)) {
        mlt_log_debug(NULL, "[openfx] propGetInt: '%s'[%d] not found\n", property, index);
        return kOfxStatErrUnknown;
    }
    *value = mlt_properties_get_int(p, property);
    return kOfxStatOK;
}

static OfxStatus propGetPointerN(OfxPropertySetHandle properties,
                                 const char *property,
                                 int count,
                                 void **value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = fetch_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        if (!mlt_properties_exists(p, property)) {
            mlt_log_debug(NULL, "[openfx] propGetPointerN: '%s'[%d] not found\n", property, i);
            return kOfxStatErrUnknown;
        }
        value[i] = mlt_properties_get_data(p, property, NULL);
    }
    return kOfxStatOK;
}

static OfxStatus propGetStringN(OfxPropertySetHandle properties,
                                const char *property,
                                int count,
                                char **value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = fetch_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        if (!mlt_properties_exists(p, property)) {
            mlt_log_debug(NULL, "[openfx] propGetStringN: '%s'[%d] not found\n", property, i);
            return kOfxStatErrUnknown;
        }
        value[i] = mlt_properties_get(p, property);
    }
    return kOfxStatOK;
}

static OfxStatus propGetDoubleN(OfxPropertySetHandle properties,
                                const char *property,
                                int count,
                                double *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = fetch_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        if (!mlt_properties_exists(p, property)) {
            mlt_log_debug(NULL, "[openfx] propGetDoubleN: '%s'[%d] not found\n", property, i);
            return kOfxStatErrUnknown;
        }
        value[i] = mlt_properties_get_double(p, property);
    }
    return kOfxStatOK;
}

static OfxStatus propGetIntN(OfxPropertySetHandle properties,
                             const char *property,
                             int count,
                             int *value)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    for (int i = 0; i < count; ++i) {
        mlt_properties p = fetch_mlt_properties(properties, i);
        if (!p)
            return kOfxStatErrBadIndex;
        if (!mlt_properties_exists(p, property)) {
            mlt_log_debug(NULL, "[openfx] propGetIntN: '%s'[%d] not found\n", property, i);
            return kOfxStatErrUnknown;
        }
        value[i] = mlt_properties_get_int(p, property);
    }
    return kOfxStatOK;
}

static OfxStatus propReset(OfxPropertySetHandle properties, const char *property)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    int i = 0;
    while (true) {
        mlt_properties p = fetch_mlt_properties(properties, i);
        if (!p)
            break;
        mlt_properties_clear(p, property);
        i++;
    }
    return kOfxStatOK;
}

static OfxStatus propGetDimension(OfxPropertySetHandle properties, const char *property, int *count)
{
    if (properties == NULL)
        return kOfxStatErrBadHandle;
    *count = 0;
    while (true) {
        mlt_properties p = fetch_mlt_properties(properties, *count);
        if (!p)
            break;
        if (mlt_properties_exists(p, property)) {
            (*count)++;
        } else {
            break;
        }
    }
    return kOfxStatOK;
}

static OfxPropertySuiteV1 MltOfxPropertySuiteV1 = {propSetPointer,
                                                   propSetString,
                                                   propSetDouble,
                                                   propSetInt,
                                                   propSetPointerN,
                                                   propSetStringN,
                                                   propSetDoubleN,
                                                   propSetIntN,
                                                   propGetPointer,
                                                   propGetString,
                                                   propGetDouble,
                                                   propGetInt,
                                                   propGetPointerN,
                                                   propGetStringN,
                                                   propGetDoubleN,
                                                   propGetIntN,
                                                   propReset,
                                                   propGetDimension};

static OfxStatus paramDefine(OfxParamSetHandle paramSet,
                             const char *paramType,
                             const char *name,
                             OfxPropertySetHandle *propertySet)
{
    mlt_log_debug(NULL, "[openfx] paramDefine: `%s`:`%s`\n", name, paramType);
    if (paramSet == NULL)
        return kOfxStatErrBadHandle;
    mlt_properties params = (mlt_properties) paramSet;
    mlt_properties p = mlt_properties_get_properties(params, name);
    if (p != NULL)
        return kOfxStatErrExists;

    if (strcmp(kOfxParamTypeInteger, paramType) == 0 || strcmp(kOfxParamTypeDouble, paramType) == 0
        || strcmp(kOfxParamTypeBoolean, paramType) == 0
        || strcmp(kOfxParamTypeChoice, paramType) == 0
        || strcmp(kOfxParamTypeStrChoice, paramType) == 0
        || strcmp(kOfxParamTypeRGBA, paramType) == 0 || strcmp(kOfxParamTypeRGB, paramType) == 0
        || strcmp(kOfxParamTypeDouble2D, paramType) == 0
        || strcmp(kOfxParamTypeInteger2D, paramType) == 0
        || strcmp(kOfxParamTypeDouble3D, paramType) == 0
        || strcmp(kOfxParamTypeInteger3D, paramType) == 0
        || strcmp(kOfxParamTypeString, paramType) == 0
        || strcmp(kOfxParamTypeCustom, paramType) == 0 || strcmp(kOfxParamTypeBytes, paramType) == 0
        || strcmp(kOfxParamTypeGroup, paramType) == 0
        // Currently page and push buttons are ignored but we need them so plugins keep feeding use with parameters
        || strcmp(kOfxParamTypePage, paramType) == 0
        || strcmp(kOfxParamTypePushButton, paramType) == 0) {
        mlt_properties pt = mlt_properties_new();
        mlt_properties_set_string(pt, "t", paramType);
        mlt_properties_set_string(pt, "identifier", name);

        mlt_properties param_props = mlt_properties_new();
        mlt_properties_set_properties(pt, "p", param_props);
        mlt_properties_close(param_props);
        mlt_properties_set_properties(params, name, pt);
        mlt_properties_close(pt);

        propSetString((OfxPropertySetHandle) param_props, kOfxParamPropType, 0, paramType);

        if (propertySet != NULL) {
            *propertySet = (OfxPropertySetHandle) param_props;
        }
    } else {
        return kOfxStatErrUnknown;
    }

    return kOfxStatOK;
}

static OfxStatus paramGetHandle(OfxParamSetHandle paramSet,
                                const char *name,
                                OfxParamHandle *param,
                                OfxPropertySetHandle *propertySet)
{
    if (!paramSet)
        return kOfxStatErrBadHandle;
    mlt_properties p = mlt_properties_get_properties((mlt_properties) paramSet, name);
    if (!p)
        return kOfxStatErrUnknown;
    *param = (OfxParamHandle) p;
    mlt_properties param_props = mlt_properties_get_properties(p, "p");
    if (param_props != NULL && propertySet != NULL) {
        *propertySet = (OfxPropertySetHandle) param_props;
    }
    return kOfxStatOK;
}

static OfxStatus paramSetGetPropertySet(OfxParamSetHandle paramSet, OfxPropertySetHandle *propHandle)
{
    if (!paramSet)
        return kOfxStatErrBadHandle;
    mlt_properties plugin_props = mlt_properties_get_properties((mlt_properties) paramSet,
                                                                "plugin_props");
    if (!plugin_props)
        return kOfxStatErrUnknown;
    *propHandle = (OfxPropertySetHandle) plugin_props;
    return kOfxStatOK;
}

static OfxStatus paramGetPropertySet(OfxParamHandle param, OfxPropertySetHandle *propHandle)
{
    if (!param)
        return kOfxStatErrBadHandle;
    mlt_properties param_props = mlt_properties_get_properties((mlt_properties) param, "p");
    if (!param_props)
        return kOfxStatErrUnknown;
    *propHandle = (OfxPropertySetHandle) param_props;
    return kOfxStatOK;
}

static void mltofx_initialize_param_value_from_default(mlt_properties param)
{
    if (!param)
        return;

    char *param_type = mlt_properties_get(param, "t");
    mlt_properties param_props = mlt_properties_get_properties(param, "p");
    if (!param_type || !param_props)
        return;

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0
        || strcmp(param_type, kOfxParamTypeInteger2D) == 0
        || strcmp(param_type, kOfxParamTypeInteger3D) == 0) {
        int count = 1;
        if (strcmp(param_type, kOfxParamTypeInteger2D) == 0)
            count = 2;
        else if (strcmp(param_type, kOfxParamTypeInteger3D) == 0)
            count = 3;
        for (int i = 0; i < count; ++i) {
            int value = 0;
            if (propGetInt((OfxPropertySetHandle) param_props, kOfxParamPropDefault, i, &value)
                == kOfxStatOK) {
                propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", i, value);
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble) == 0
               || strcmp(param_type, kOfxParamTypeDouble2D) == 0
               || strcmp(param_type, kOfxParamTypeDouble3D) == 0
               || strcmp(param_type, kOfxParamTypeRGB) == 0
               || strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        int count = 1;
        if (strcmp(param_type, kOfxParamTypeDouble2D) == 0)
            count = 2;
        else if (strcmp(param_type, kOfxParamTypeDouble3D) == 0
                 || strcmp(param_type, kOfxParamTypeRGB) == 0)
            count = 3;
        else if (strcmp(param_type, kOfxParamTypeRGBA) == 0)
            count = 4;
        for (int i = 0; i < count; ++i) {
            double value = 0.0;
            if (propGetDouble((OfxPropertySetHandle) param_props, kOfxParamPropDefault, i, &value)
                == kOfxStatOK) {
                propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", i, value);
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeString) == 0
               || strcmp(param_type, kOfxParamTypeStrChoice) == 0) {
        char *value = NULL;
        if (propGetString((OfxPropertySetHandle) param_props, kOfxParamPropDefault, 0, &value)
                == kOfxStatOK
            && value) {
            propSetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
        }
    } else if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
        int default_index = 0;
        if (propGetInt((OfxPropertySetHandle) param_props, kOfxParamPropDefault, 0, &default_index)
            == kOfxStatOK) {
            char *label = NULL;
            if (propGetString((OfxPropertySetHandle) param_props,
                              kOfxParamPropChoiceOption,
                              default_index,
                              &label)
                    == kOfxStatOK
                && label) {
                propSetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, label);
            }
        }
    }
}

static void mltofx_initialize_param_values(mlt_properties iparams)
{
    if (!iparams)
        return;

    int count = mlt_properties_count(iparams);
    for (int i = 1; i < count; ++i) {
        char *name = mlt_properties_get_name(iparams, i);
        if (!name)
            continue;
        mlt_properties param = mlt_properties_get_properties(iparams, name);
        mltofx_initialize_param_value_from_default(param);
    }
}

static OfxStatus paramGetValueImpl(OfxParamHandle paramHandle, va_list ap)
{
    mlt_properties param = (mlt_properties) paramHandle;
    char *param_type = mlt_properties_get(param, "t");
    mlt_properties param_props = mlt_properties_get_properties(param, "p");

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
        int *value = va_arg(ap, int *);
        OfxStatus status
            = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
        if (status != kOfxStatOK) {
            status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
            if (status != kOfxStatOK)
                *value = 0;
        }
    } else if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
        // MltOfxParamValue stores the string label; convert to integer index for the OFX plugin.
        int *value = va_arg(ap, int *);
        char *label = NULL;
        OfxStatus status
            = propGetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &label);
        if (status == kOfxStatOK && label) {
            int count = 0;
            propGetDimension((OfxPropertySetHandle) param_props, kOfxParamPropChoiceOption, &count);
            int found = 0;
            for (int i = 0; i < count; i++) {
                char *option = NULL;
                propGetString((OfxPropertySetHandle) param_props,
                              kOfxParamPropChoiceOption,
                              i,
                              &option);
                if (option && strcmp(option, label) == 0) {
                    *value = i;
                    found = 1;
                    break;
                }
            }
            if (!found) {
                status = propGetInt((OfxPropertySetHandle) param_props,
                                    "OfxParamPropDefault",
                                    0,
                                    value);
                if (status != kOfxStatOK)
                    *value = 0;
            }
        } else {
            status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
            if (status != kOfxStatOK)
                *value = 0;
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
        double *value = va_arg(ap, double *);
        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
        if (status != kOfxStatOK) {
            status = propGetDouble((OfxPropertySetHandle) param_props,
                                   "OfxParamPropDefault",
                                   0,
                                   value);
            if (status != kOfxStatOK)
                *value = 0.0;
        }
    } else if (strcmp(param_type, kOfxParamTypeString) == 0
               || strcmp(param_type, kOfxParamTypeStrChoice) == 0) {
        char **value = va_arg(ap, char **);
        OfxStatus status
            = propGetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
        if (status != kOfxStatOK) {
            status = propGetString((OfxPropertySetHandle) param_props,
                                   "OfxParamPropDefault",
                                   0,
                                   value);
            if (status != kOfxStatOK)
                *value = "";
        }
    } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        double *red = va_arg(ap, double *);
        double *green = va_arg(ap, double *);
        double *blue = va_arg(ap, double *);
        double *alpha = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, alpha);

        if (status != kOfxStatOK) {
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, red);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       1,
                                       green);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       2,
                                       blue);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       3,
                                       alpha);
            if (status != kOfxStatOK) {
                *red = 0.0;
                *green = 0.0;
                *blue = 0.0;
                *alpha = 1.0;
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
        double *red = va_arg(ap, double *);
        double *green = va_arg(ap, double *);
        double *blue = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);

        if (status != kOfxStatOK) {
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, red);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       1,
                                       green);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       2,
                                       blue);
            if (status != kOfxStatOK) {
                *red = 0.0;
                *green = 0.0;
                *blue = 0.0;
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
        double *X = va_arg(ap, double *);
        double *Y = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, X);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, Y);

        if (status != kOfxStatOK) {
            status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, X);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       1,
                                       Y);
            if (status != kOfxStatOK) {
                *X = 0.0;
                *Y = 0.0;
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeInteger2D) == 0) {
        int *X = va_arg(ap, int *);
        int *Y = va_arg(ap, int *);

        OfxStatus status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, X);
        if (status == kOfxStatOK)
            status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, Y);

        if (status != kOfxStatOK) {
            status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, X);
            if (status == kOfxStatOK)
                status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 1, Y);
            if (status != kOfxStatOK) {
                *X = 0;
                *Y = 0;
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble3D) == 0) {
        double *X = va_arg(ap, double *);
        double *Y = va_arg(ap, double *);
        double *Z = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, X);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, Y);
        if (status == kOfxStatOK)
            status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, Z);

        if (status != kOfxStatOK) {
            status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, X);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       1,
                                       Y);
            if (status == kOfxStatOK)
                status = propGetDouble((OfxPropertySetHandle) param_props,
                                       "OfxParamPropDefault",
                                       2,
                                       Z);
            if (status != kOfxStatOK) {
                *X = 0.0;
                *Y = 0.0;
                *Z = 0.0;
            }
        }
    } else if (strcmp(param_type, kOfxParamTypeInteger3D) == 0) {
        int *X = va_arg(ap, int *);
        int *Y = va_arg(ap, int *);
        int *Z = va_arg(ap, int *);

        OfxStatus status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, X);
        if (status == kOfxStatOK)
            status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, Y);
        if (status == kOfxStatOK)
            status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, Z);

        if (status != kOfxStatOK) {
            status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, X);
            if (status == kOfxStatOK)
                status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 1, Y);
            if (status == kOfxStatOK)
                status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 2, Z);
            if (status != kOfxStatOK) {
                *X = 0;
                *Y = 0;
                *Z = 0;
            }
        }
    }

    return kOfxStatOK;
}

static OfxStatus paramGetValue(OfxParamHandle paramHandle, ...)
{
    if (!paramHandle)
        return kOfxStatErrBadHandle;
    va_list ap;
    va_start(ap, paramHandle);
    OfxStatus status = paramGetValueImpl(paramHandle, ap);
    va_end(ap);
    return status;
}

static OfxStatus paramGetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...)
{
    if (!paramHandle)
        return kOfxStatErrBadHandle;
    va_list ap;
    va_start(ap, time);
    OfxStatus status = paramGetValueImpl(paramHandle, ap);
    va_end(ap);
    return status;
}

static OfxStatus paramGetDerivative(OfxParamHandle paramHandle, OfxTime time, ...)
{
    return kOfxStatErrUnsupported;
}

static OfxStatus paramGetIntegral(OfxParamHandle paramHandle, OfxTime time1, OfxTime time2, ...)
{
    return kOfxStatErrUnsupported;
}

static uint8_t mltofx_double_to_color_component(double value)
{
    if (!(value >= 0.0))
        value = 0.0;
    else if (value > 1.0)
        value = 1.0;
    return (uint8_t) lrint(value * 255.0);
}

static void mltofx_sync_filter_property_from_param(mlt_properties param, mlt_properties param_props)
{
    mlt_properties filter_properties = mlt_properties_get_data(param_props,
                                                               "_filter_properties",
                                                               NULL);
    char *param_name = mlt_properties_get(param, "identifier");
    char *param_type = mlt_properties_get(param, "t");
    if (!filter_properties || !param_name || !param_type)
        return;

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
        int value = 0;
        if (propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &value)
            == kOfxStatOK)
            mlt_properties_set_int(filter_properties, param_name, value);
    } else if (strcmp(param_type, kOfxParamTypeChoice) == 0
               || strcmp(param_type, kOfxParamTypeString) == 0
               || strcmp(param_type, kOfxParamTypeStrChoice) == 0) {
        char *value = NULL;
        if (propGetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &value)
                == kOfxStatOK
            && value) {
            mlt_properties_set(filter_properties, param_name, value);
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
        double value = 0.0;
        if (propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &value)
            == kOfxStatOK) {
            mlt_properties_set_double(filter_properties, param_name, value);
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
        double x = 0.0;
        double y = 0.0;
        if (propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &x)
                == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, &y)
                   == kOfxStatOK) {
            char value[90] = "";
            snprintf(value, sizeof(value), "%.4f %.4f", x, y);
            mlt_properties_set(filter_properties, param_name, value);
        }
    } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        double red = 0.0;
        double green = 0.0;
        double blue = 0.0;
        double alpha = 1.0;
        if (propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &red)
                == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, &green)
                   == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, &blue)
                   == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, &alpha)
                   == kOfxStatOK) {
            mlt_color value = {mltofx_double_to_color_component(red),
                               mltofx_double_to_color_component(green),
                               mltofx_double_to_color_component(blue),
                               mltofx_double_to_color_component(alpha)};
            mlt_properties_set_color(filter_properties, param_name, value);
        }
    } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
        double red = 0.0;
        double green = 0.0;
        double blue = 0.0;
        if (propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, &red)
                == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, &green)
                   == kOfxStatOK
            && propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, &blue)
                   == kOfxStatOK) {
            mlt_color value = {mltofx_double_to_color_component(red),
                               mltofx_double_to_color_component(green),
                               mltofx_double_to_color_component(blue),
                               255};
            mlt_properties_set_color(filter_properties, param_name, value);
        }
    }
}

static OfxStatus paramSetValueImpl(OfxParamHandle paramHandle, va_list ap)
{
    mlt_properties param = (mlt_properties) paramHandle;
    char *param_type = mlt_properties_get(param, "t");
    mlt_properties param_props = mlt_properties_get_properties(param, "p");

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
        int value = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } else if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
        // OFX plugin passes an integer index; convert to string label for MltOfxParamValue.
        int index = va_arg(ap, int);
        char *label = NULL;
        OfxStatus status = propGetString((OfxPropertySetHandle) param_props,
                                         kOfxParamPropChoiceOption,
                                         index,
                                         &label);
        if (status != kOfxStatOK || !label)
            return status != kOfxStatOK ? status : kOfxStatErrBadIndex;
        propSetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, label);
    } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
        double value = va_arg(ap, double);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        double red = va_arg(ap, double);
        double green = va_arg(ap, double);
        double blue = va_arg(ap, double);
        double alpha = va_arg(ap, double);

        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, alpha);
    } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
        double red = va_arg(ap, double);
        double green = va_arg(ap, double);
        double blue = va_arg(ap, double);

        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
    } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
        double x = va_arg(ap, double);
        double y = va_arg(ap, double);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, x);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, y);
    } else if (strcmp(param_type, kOfxParamTypeInteger2D) == 0) {
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, x);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, y);
    } else if (strcmp(param_type, kOfxParamTypeDouble3D) == 0) {
        double x = va_arg(ap, double);
        double y = va_arg(ap, double);
        double z = va_arg(ap, double);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, x);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, y);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, z);
    } else if (strcmp(param_type, kOfxParamTypeInteger3D) == 0) {
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        int z = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, x);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, y);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, z);
    } else if (strcmp(param_type, kOfxParamTypeString) == 0
               || strcmp(param_type, kOfxParamTypeStrChoice) == 0) {
        char *value = va_arg(ap, char *);
        propSetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    }

    mltofx_sync_filter_property_from_param(param, param_props);

    return kOfxStatOK;
}

static OfxStatus paramSetValue(OfxParamHandle paramHandle, ...)
{
    if (!paramHandle)
        return kOfxStatErrBadHandle;
    va_list ap;
    va_start(ap, paramHandle);
    OfxStatus status = paramSetValueImpl(paramHandle, ap);
    va_end(ap);
    return status;
}

static OfxStatus paramSetValueAtTime(OfxParamHandle paramHandle, OfxTime time, ...)
{
    mlt_log_debug(NULL, "[openfx] paramSetValueAtTime\n");
    if (!paramHandle)
        return kOfxStatErrBadHandle;
    va_list ap;
    va_start(ap, time);
    OfxStatus status = paramSetValueImpl(paramHandle, ap);
    va_end(ap);
    return status;
}

static OfxStatus paramGetNumKeys(OfxParamHandle paramHandle, unsigned int *numberOfKeys)
{
    if (numberOfKeys)
        *numberOfKeys = 0;
    return kOfxStatOK;
}

static OfxStatus paramGetKeyTime(OfxParamHandle paramHandle, unsigned int nthKey, OfxTime *time)
{
    return kOfxStatErrUnsupported;
}

static OfxStatus paramGetKeyIndex(OfxParamHandle paramHandle,
                                  OfxTime time,
                                  int direction,
                                  int *index)
{
    return kOfxStatErrUnsupported;
}

static OfxStatus paramDeleteKey(OfxParamHandle paramHandle, OfxTime time)
{
    return kOfxStatOK;
}

static OfxStatus paramDeleteAllKeys(OfxParamHandle paramHandle)
{
    return kOfxStatOK;
}

static OfxStatus paramCopy(OfxParamHandle paramTo,
                           OfxParamHandle paramFrom,
                           OfxTime dstOffset,
                           const OfxRangeD *frameRange)
{
    return kOfxStatOK;
}

static OfxStatus paramEditBegin(OfxParamSetHandle paramSet, const char *name)
{
    return kOfxStatOK;
}

static OfxStatus paramEditEnd(OfxParamSetHandle paramSet)
{
    return kOfxStatOK;
}

static OfxParameterSuiteV1 MltOfxParameterSuiteV1 = {paramDefine,
                                                     paramGetHandle,
                                                     paramSetGetPropertySet,
                                                     paramGetPropertySet,
                                                     paramGetValue,
                                                     paramGetValueAtTime,
                                                     paramGetDerivative,
                                                     paramGetIntegral,
                                                     paramSetValue,
                                                     paramSetValueAtTime,
                                                     paramGetNumKeys,
                                                     paramGetKeyTime,
                                                     paramGetKeyIndex,
                                                     paramDeleteKey,
                                                     paramDeleteAllKeys,
                                                     paramCopy,
                                                     paramEditBegin,
                                                     paramEditEnd};

static OfxStatus memoryAlloc(void *handle, size_t nBytes, void **allocatedData)
{
    void *temp = calloc(1, nBytes);
    if (!temp)
        return kOfxStatErrMemory;
    *allocatedData = temp;
    return kOfxStatOK;
}

static OfxStatus memoryFree(void *allocatedData)
{
    free(allocatedData);
    return kOfxStatOK;
}

static OfxMemorySuiteV1 MltOfxMemorySuiteV1 = {memoryAlloc, memoryFree};

// --- Thread-local storage for OFX threading context ---
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
static _Thread_local unsigned int ofx_thread_index = 0;
static _Thread_local int ofx_thread_spawned = 0;
static _Thread_local unsigned int ofx_multi_thread_depth = 0;
#else
static pthread_key_t ofx_thread_index_key;
static pthread_key_t ofx_thread_spawned_key;
static pthread_key_t ofx_multi_thread_depth_key;
static pthread_once_t ofx_thread_keys_once = PTHREAD_ONCE_INIT;
static void ofx_make_thread_keys(void)
{
    pthread_key_create(&ofx_thread_index_key, NULL);
    pthread_key_create(&ofx_thread_spawned_key, NULL);
    pthread_key_create(&ofx_multi_thread_depth_key, NULL);
}
static void ofx_set_thread_index(unsigned int idx)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    pthread_setspecific(ofx_thread_index_key, (void *) (uintptr_t) idx);
}
static void ofx_set_thread_spawned(int val)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    pthread_setspecific(ofx_thread_spawned_key, (void *) (uintptr_t) val);
}
static unsigned int ofx_get_thread_index(void)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    return (unsigned int) (uintptr_t) pthread_getspecific(ofx_thread_index_key);
}
static int ofx_get_thread_spawned(void)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    return (int) (uintptr_t) pthread_getspecific(ofx_thread_spawned_key);
}
static void ofx_set_multi_thread_depth(unsigned int depth)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    pthread_setspecific(ofx_multi_thread_depth_key, (void *) (uintptr_t) depth);
}
static unsigned int ofx_get_multi_thread_depth(void)
{
    pthread_once(&ofx_thread_keys_once, ofx_make_thread_keys);
    return (unsigned int) (uintptr_t) pthread_getspecific(ofx_multi_thread_depth_key);
}
#endif

typedef struct
{
    OfxThreadFunctionV1 *func;
    unsigned int nThreads;
    void *customArg;
} OfxSlicesJob;

static int ofx_slices_proc(int id, int idx, int jobs, void *cookie)
{
    (void) id; // unused
    OfxSlicesJob *job = (OfxSlicesJob *) cookie;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    ofx_thread_index = idx;
    ofx_thread_spawned = 1;
    ofx_multi_thread_depth++;
#else
    ofx_set_thread_index(idx);
    ofx_set_thread_spawned(1);
    ofx_set_multi_thread_depth(ofx_get_multi_thread_depth() + 1);
#endif
    job->func(idx, jobs, job->customArg);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    ofx_multi_thread_depth--;
    ofx_thread_spawned = 0;
#else
    ofx_set_multi_thread_depth(ofx_get_multi_thread_depth() - 1);
    ofx_set_thread_spawned(0);
#endif
    return 0;
}

static OfxStatus multiThread(OfxThreadFunctionV1 func, unsigned int nThreads, void *customArg)
{
    unsigned int maxThreads;

    if (!func)
        return kOfxStatFailed;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    if (ofx_multi_thread_depth)
        return kOfxStatErrExists;
#else
    if (ofx_get_multi_thread_depth())
        return kOfxStatErrExists;
#endif
    mlt_log_debug(NULL, "[openfx] multiThread: func=%p nThreads=%u\n", func, nThreads);
    maxThreads = (unsigned int) mlt_slices_count_normal();
    if (maxThreads < 1)
        maxThreads = 1;
    if (nThreads == 0 || nThreads > maxThreads)
        nThreads = maxThreads;
    if (nThreads == 1) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        ofx_thread_index = 0;
        ofx_thread_spawned = 1;
        ofx_multi_thread_depth++;
#else
        ofx_set_thread_index(0);
        ofx_set_thread_spawned(1);
        ofx_set_multi_thread_depth(ofx_get_multi_thread_depth() + 1);
#endif
        func(0, 1, customArg);
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        ofx_multi_thread_depth--;
        ofx_thread_spawned = 0;
#else
        ofx_set_multi_thread_depth(ofx_get_multi_thread_depth() - 1);
        ofx_set_thread_spawned(0);
#endif
        return kOfxStatOK;
    }
    OfxSlicesJob job = {func, nThreads, customArg};
    mlt_slices_run_normal(nThreads, ofx_slices_proc, &job);
    return kOfxStatOK;
}

static OfxStatus multiThreadNumCPUs(unsigned int *nCPUs)
{
    if (!nCPUs)
        return kOfxStatFailed;
    int count = mlt_slices_count_normal();
    *nCPUs = count > 0 ? (unsigned int) count : 1;
    return kOfxStatOK;
}

static OfxStatus multiThreadIndex(unsigned int *threadIndex)
{
    if (!threadIndex)
        return kOfxStatFailed;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    *threadIndex = ofx_thread_spawned ? ofx_thread_index : 0;
#else
    *threadIndex = ofx_get_thread_spawned() ? ofx_get_thread_index() : 0;
#endif
    return kOfxStatOK;
}

int multiThreadIsSpawnedThread(void)
{
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    return ofx_thread_spawned;
#else
    return ofx_get_thread_spawned();
#endif
}

typedef struct
{
    pthread_mutex_t mtx;
} OfxMutexImpl;

static OfxStatus mutexCreate(OfxMutexHandle *mutex, int lockCount)
{
    if (!mutex)
        return kOfxStatFailed;
    if (lockCount < 0)
        return kOfxStatErrValue;
    OfxMutexImpl *m = (OfxMutexImpl *) malloc(sizeof(OfxMutexImpl));
    if (!m)
        return kOfxStatErrMemory;
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    int err = pthread_mutex_init(&m->mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    if (err != 0) {
        free(m);
        return kOfxStatErrMemory;
    }
    *mutex = (OfxMutexHandle) m;
    for (int i = 0; i < lockCount; ++i) {
        err = pthread_mutex_lock(&m->mtx);
        if (err != 0) {
            while (i-- > 0)
                pthread_mutex_unlock(&m->mtx);
            pthread_mutex_destroy(&m->mtx);
            free(m);
            *mutex = NULL;
            return kOfxStatFailed;
        }
    }
    return kOfxStatOK;
}

static OfxStatus mutexDestroy(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    OfxMutexImpl *m = (OfxMutexImpl *) mutex;
    int err = pthread_mutex_destroy(&m->mtx);
    if (err)
        return kOfxStatErrBadHandle;
    free(m);
    return kOfxStatOK;
}

static OfxStatus mutexLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    OfxMutexImpl *m = (OfxMutexImpl *) mutex;
    int err = pthread_mutex_lock(&m->mtx);
    return err == 0 ? kOfxStatOK : kOfxStatErrBadHandle;
}

static OfxStatus mutexUnLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    OfxMutexImpl *m = (OfxMutexImpl *) mutex;
    int err = pthread_mutex_unlock(&m->mtx);
    return err == 0 ? kOfxStatOK : kOfxStatErrBadHandle;
}

static OfxStatus mutexTryLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    OfxMutexImpl *m = (OfxMutexImpl *) mutex;
    int err = pthread_mutex_trylock(&m->mtx);
    if (err == 0)
        return kOfxStatOK;
    else if (err == EBUSY)
        return kOfxStatFailed;
    else
        return kOfxStatErrBadHandle;
}

static OfxMultiThreadSuiteV1 MltOfxMultiThreadSuiteV1 = {multiThread,
                                                         multiThreadNumCPUs,
                                                         multiThreadIndex,
                                                         multiThreadIsSpawnedThread,
                                                         mutexCreate,
                                                         mutexDestroy,
                                                         mutexLock,
                                                         mutexUnLock,
                                                         mutexTryLock};

static void mltofx_log_message(const char *kind,
                               const char *messageType,
                               const char *messageId,
                               const char *format,
                               va_list ap)
{
    char formatted[1024] = "";
    const char *type = messageType ? messageType : "OfxMessageUnknown";
    const char *id = (messageId && messageId[0]) ? messageId : NULL;

    if (format && format[0]) {
        vsnprintf(formatted, sizeof(formatted), format, ap);
    }

    if (strcmp(type, "OfxMessageFatal") == 0) {
        mlt_log_fatal(NULL,
                      "[openfx] %s type=%s%s%s%s%s\n",
                      kind,
                      type,
                      id ? " id=" : "",
                      id ? id : "",
                      formatted[0] ? " msg=" : "",
                      formatted);
    } else if (strcmp(type, "OfxMessageError") == 0) {
        mlt_log_error(NULL,
                      "[openfx] %s type=%s%s%s%s%s\n",
                      kind,
                      type,
                      id ? " id=" : "",
                      id ? id : "",
                      formatted[0] ? " msg=" : "",
                      formatted);
    } else if (strcmp(type, "OfxMessageWarning") == 0) {
        mlt_log_warning(NULL,
                        "[openfx] %s type=%s%s%s%s%s\n",
                        kind,
                        type,
                        id ? " id=" : "",
                        id ? id : "",
                        formatted[0] ? " msg=" : "",
                        formatted);
    } else if (strcmp(type, "OfxMessageLog") == 0) {
        mlt_log_verbose(NULL,
                        "[openfx] %s type=%s%s%s%s%s\n",
                        kind,
                        type,
                        id ? " id=" : "",
                        id ? id : "",
                        formatted[0] ? " msg=" : "",
                        formatted);
    } else {
        mlt_log_info(NULL,
                     "[openfx] %s type=%s%s%s%s%s\n",
                     kind,
                     type,
                     id ? " id=" : "",
                     id ? id : "",
                     formatted[0] ? " msg=" : "",
                     formatted);
    }
}

static OfxStatus message(
    void *handle, const char *messageType, const char *messageId, const char *format, ...)
{
    (void) handle;
    va_list ap;
    va_start(ap, format);
    mltofx_log_message("message", messageType, messageId, format, ap);
    va_end(ap);
    return kOfxStatOK;
}

static OfxMessageSuiteV1 MltOfxMessageSuiteV1 = {message};

static OfxStatus setPersistentMessage(
    void *handle, const char *messageType, const char *messageId, const char *format, ...)
{
    (void) handle;
    va_list ap;
    va_start(ap, format);
    mltofx_log_message("persistent", messageType, messageId, format, ap);
    va_end(ap);
    return kOfxStatOK;
}

static OfxStatus clearPersistentMessage(void *handle)
{
    (void) handle;
    mlt_log_debug(NULL, "[openfx] persistent message cleared\n");
    return kOfxStatOK;
}

static OfxMessageSuiteV2 MltOfxMessageSuiteV2 = {message, // Same as the V1 message suite call.
                                                 setPersistentMessage,
                                                 clearPersistentMessage};

static OfxStatus interactSwapBuffers(OfxInteractHandle interactInstance)
{
    return kOfxStatOK;
}

static OfxStatus interactRedraw(OfxInteractHandle interactInstance)
{
    return kOfxStatOK;
}

static OfxStatus interactGetPropertySet(OfxInteractHandle interactInstance,
                                        OfxPropertySetHandle *property)
{
    return kOfxStatErrUnsupported;
}

static OfxInteractSuiteV1 MltOfxInteractSuiteV1 = {interactSwapBuffers,
                                                   interactRedraw,
                                                   interactGetPropertySet};

/*
static OfxStatus getColour(OfxDrawContextHandle context,
                            OfxStandardColour std_colour,
                            OfxRGBAColourF *colour)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxStatus setColour(OfxDrawContextHandle context, const OfxRGBAColourF *colour)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxStatus setLineWidth(OfxDrawContextHandle context, float width)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxStatus setLineStipple(OfxDrawContextHandle context, OfxDrawLineStipplePattern pattern)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxStatus draw(OfxDrawContextHandle context,
                        OfxDrawPrimitive primitive,
                        const OfxPointD *points,
                        int point_count)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxStatus drawText(OfxDrawContextHandle context,
                            const char *text,
                            const OfxPointD *pos,
                            int alignment)
{
    mlt_log_debug(NULL, "[openfx] OfxDrawSuite `%s`\n", __FUNCTION__);
    return kOfxStatOK;
}

static OfxDrawSuiteV1 MltOfxDrawSuiteV1
    = {getColour, setColour, setLineWidth, setLineStipple, draw, drawText};
*/
static OfxStatus progressStartV1(void *effectInstance, const char *label)
{
    return kOfxStatOK;
}

static OfxStatus progressUpdateV1(void *effectInstance, double progress)
{
    return kOfxStatOK;
}

static OfxStatus progressEndV1(void *effectInstance)
{
    return kOfxStatOK;
}

static OfxProgressSuiteV1 MltOfxProgressSuiteV1 = {progressStartV1, progressUpdateV1, progressEndV1};

static OfxStatus progressStartV2(void *effectInstance, const char *message, const char *messageid)
{
    return kOfxStatOK;
}

static OfxStatus progressUpdateV2(void *effectInstance, double progress)
{
    return kOfxStatOK;
}

static OfxStatus progressEndV2(void *effectInstance)
{
    return kOfxStatOK;
}

static OfxProgressSuiteV2 MltOfxProgressSuiteV2 = {progressStartV2, progressUpdateV2, progressEndV2};

static OfxStatus getTime(void *instance, double *time)
{
    return kOfxStatErrUnsupported;
}

static OfxStatus gotoTime(void *instance, double time)
{
    return kOfxStatOK;
}

static OfxStatus getTimeBounds(void *instance, double *firstTime, double *lastTime)
{
    return kOfxStatErrUnsupported;
}

static OfxTimeLineSuiteV1 MltOfxTimeLineSuiteV1 = {getTime, gotoTime, getTimeBounds};

static OfxStatus parametricParamGetValue(OfxParamHandle param,
                                         int curveIndex,
                                         OfxTime time,
                                         double parametricPosition,
                                         double *returnValue)
{
    if (returnValue)
        *returnValue = 0.0;
    return kOfxStatOK;
}

static OfxStatus parametricParamGetNControlPoints(OfxParamHandle param,
                                                  int curveIndex,
                                                  double time,
                                                  int *returnValue)
{
    if (returnValue)
        *returnValue = 0;
    return kOfxStatOK;
}

static OfxStatus parametricParamGetNthControlPoint(
    OfxParamHandle param, int curveIndex, double time, int nthCtl, double *key, double *value)
{
    return kOfxStatErrUnsupported;
}

static OfxStatus parametricParamSetNthControlPoint(OfxParamHandle param,
                                                   int curveIndex,
                                                   double time,
                                                   int nthCtl,
                                                   double key,
                                                   double value,
                                                   bool addAnimationKey)
{
    return kOfxStatOK;
}

static OfxStatus parametricParamAddControlPoint(OfxParamHandle param,
                                                int curveIndex,
                                                double time,
                                                double key,
                                                double value,
                                                bool addAnimationKey)
{
    return kOfxStatOK;
}

static OfxStatus parametricParamDeleteControlPoint(OfxParamHandle param, int curveIndex, int nthCtl)
{
    return kOfxStatOK;
}

static OfxStatus parametricParamDeleteAllControlPoints(OfxParamHandle param, int curveIndex)
{
    return kOfxStatOK;
}

static OfxParametricParameterSuiteV1 MltOfxParametricParameterSuiteV1
    = {parametricParamGetValue,
       parametricParamGetNControlPoints,
       parametricParamGetNthControlPoint,
       parametricParamSetNthControlPoint,
       parametricParamAddControlPoint,
       parametricParamDeleteControlPoint,
       parametricParamDeleteAllControlPoints};

/*
static OfxStatus clipLoadTexture(OfxImageClipHandle clip,
                OfxTime time,
                const char *format,
                const OfxRectD *region,
                OfxPropertySetHandle *textureHandle)
{
    return kOfxStatOK;
}

static OfxStatus clipFreeTexture(OfxPropertySetHandle textureHandle)
{
    return kOfxStatOK;
}

static OfxStatus flushResources()
{
    return kOfxStatOK;
}

static OfxImageEffectOpenGLRenderSuiteV1 MltOfxOpenGLRenderSuiteV1
    = {clipLoadTexture,
        clipFreeTexture,
        flushResources};
*/

static void mltofx_log_status_code(OfxStatus code, char *msg)
{
    mlt_log_debug(NULL, "[openfx] output of `%s` is ", msg);
    switch (code) {
    case kOfxStatOK:
        mlt_log_debug(NULL, "kOfxStatOK\n");
        break;

    case kOfxStatFailed:
        mlt_log_debug(NULL, "kOfxStatFailed\n");
        break;

    case kOfxStatErrFatal:
        mlt_log_debug(NULL, "kOfxStatErrFatal\n");
        break;

    case kOfxStatErrUnknown:
        mlt_log_debug(NULL, "kOfxStatErrUnknown\n");
        break;

    case kOfxStatErrMissingHostFeature:
        mlt_log_debug(NULL, "kOfxStatErrMissingHostFeature\n");
        break;

    case kOfxStatErrUnsupported:
        mlt_log_debug(NULL, "kOfxStatErrUnsupported\n");
        break;

    case kOfxStatErrExists:
        mlt_log_debug(NULL, "kOfxStatErrExists\n");
        break;

    case kOfxStatErrFormat:
        mlt_log_debug(NULL, "kOfxStatErrFormat\n");
        break;

    case kOfxStatErrMemory:
        mlt_log_debug(NULL, "kOfxStatErrMemory\n");
        break;

    case kOfxStatErrBadHandle:
        mlt_log_debug(NULL, "kOfxStatErrBadHandle\n");
        break;

    case kOfxStatErrBadIndex:
        mlt_log_debug(NULL, "kOfxStatErrBadIndex\n");
        break;

    case kOfxStatErrValue:
        mlt_log_debug(NULL, "kOfxStatErrValue\n");
        break;

    case kOfxStatReplyYes:
        mlt_log_debug(NULL, "kOfxStatReplyYes\n");
        break;

    case kOfxStatReplyNo:
        mlt_log_debug(NULL, "kOfxStatReplyNo\n");
        break;

    case kOfxStatReplyDefault:
        mlt_log_debug(NULL, "kOfxStatReplyDefault\n");
        break;

    default:
        break;
    }
    mlt_log_debug(NULL, "\n");
}

static void mltofx_make_clip_pref_key(char *key,
                                      size_t key_size,
                                      const char *prefix,
                                      const char *clip_name)
{
    snprintf(key, key_size, "%s%s", prefix, clip_name);
}

static void mltofx_prime_clip_preferences(mlt_properties image_effect, mlt_properties out_args)
{
    if (!image_effect || !out_args)
        return;

    mlt_properties clips = mlt_properties_get_properties(image_effect, "clips");
    int clips_count = clips ? mlt_properties_count(clips) : 0;

    for (int i = 0; i < clips_count; ++i) {
        char *clip_name = mlt_properties_get_name(clips, i);
        if (!clip_name)
            continue;

        mlt_properties clip = NULL;
        mlt_properties clip_props = NULL;
        clipGetHandle((OfxImageEffectHandle) image_effect,
                      clip_name,
                      (OfxImageClipHandle *) &clip,
                      (OfxPropertySetHandle *) &clip_props);
        if (!clip_props)
            continue;

        char key[256] = {'\0'};
        char *str_value = NULL;
        double dbl_value = 0.0;

        mltofx_make_clip_pref_key(key, sizeof(key), "OfxImageClipPropComponents_", clip_name);
        if (propGetString((OfxPropertySetHandle) clip_props,
                          kOfxImageEffectPropComponents,
                          0,
                          &str_value)
                == kOfxStatOK
            && str_value) {
            propSetString((OfxPropertySetHandle) out_args, key, 0, str_value);
        }

        mltofx_make_clip_pref_key(key, sizeof(key), "OfxImageClipPropDepth_", clip_name);
        if (propGetString((OfxPropertySetHandle) clip_props,
                          kOfxImageEffectPropPixelDepth,
                          0,
                          &str_value)
                == kOfxStatOK
            && str_value) {
            propSetString((OfxPropertySetHandle) out_args, key, 0, str_value);
        }

        mltofx_make_clip_pref_key(key, sizeof(key), "OfxImageClipPropPAR_", clip_name);
        if (propGetDouble((OfxPropertySetHandle) clip_props,
                          kOfxImagePropPixelAspectRatio,
                          0,
                          &dbl_value)
            == kOfxStatOK) {
            propSetDouble((OfxPropertySetHandle) out_args, key, 0, dbl_value);
        }
    }

    mlt_properties output_clip = NULL;
    mlt_properties output_clip_props = NULL;
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &output_clip,
                  (OfxPropertySetHandle *) &output_clip_props);

    if (output_clip_props) {
        int int_value = 0;

        propSetString((OfxPropertySetHandle) out_args,
                      kOfxImageEffectPropPreMultiplication,
                      0,
                      kOfxImageUnPreMultiplied);

        char *str_value = NULL;
        if (propGetString((OfxPropertySetHandle) output_clip_props,
                          kOfxImageClipPropFieldOrder,
                          0,
                          &str_value)
                == kOfxStatOK
            && str_value) {
            propSetString((OfxPropertySetHandle) out_args,
                          kOfxImageClipPropFieldOrder,
                          0,
                          str_value);
        }
        if (propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImageClipPropContinuousSamples,
                       0,
                       &int_value)
            == kOfxStatOK) {
            propSetInt((OfxPropertySetHandle) out_args,
                       kOfxImageClipPropContinuousSamples,
                       0,
                       int_value);
        } else {
            propSetInt((OfxPropertySetHandle) out_args, kOfxImageClipPropContinuousSamples, 0, 0);
        }
    }

    mlt_properties effect_props = mlt_properties_get_properties(image_effect, "props");
    if (effect_props) {
        int frame_varying = 0;
        if (propGetInt((OfxPropertySetHandle) effect_props,
                       kOfxImageEffectFrameVarying,
                       0,
                       &frame_varying)
            == kOfxStatOK) {
            propSetInt((OfxPropertySetHandle) out_args,
                       kOfxImageEffectFrameVarying,
                       0,
                       frame_varying);
        } else {
            propSetInt((OfxPropertySetHandle) out_args, kOfxImageEffectFrameVarying, 0, 0);
        }
    }
}

static void mltofx_apply_clip_preferences(mlt_properties image_effect, mlt_properties pref_args)
{
    if (!image_effect || !pref_args)
        return;

    mlt_properties output_clip = NULL;
    mlt_properties output_clip_props = NULL;
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &output_clip,
                  (OfxPropertySetHandle *) &output_clip_props);

    if (output_clip_props) {
        int int_value = 0;
        propSetString((OfxPropertySetHandle) output_clip_props,
                      kOfxImageEffectPropPreMultiplication,
                      0,
                      kOfxImageUnPreMultiplied);
        if (propGetInt((OfxPropertySetHandle) pref_args,
                       kOfxImageClipPropContinuousSamples,
                       0,
                       &int_value)
            == kOfxStatOK) {
            propSetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImageClipPropContinuousSamples,
                       0,
                       int_value);
        }
    }

    mlt_properties effect_props = mlt_properties_get_properties(image_effect, "props");
    if (effect_props) {
        int int_value = 0;
        if (propGetInt((OfxPropertySetHandle) pref_args, kOfxImageEffectFrameVarying, 0, &int_value)
            == kOfxStatOK) {
            propSetInt((OfxPropertySetHandle) effect_props,
                       kOfxImageEffectFrameVarying,
                       0,
                       int_value);
        }
    }
}

static void mltofx_apply_cached_clip_preferences(mlt_properties image_effect)
{
    if (!image_effect)
        return;
    mlt_properties pref_args = mlt_properties_get_properties(image_effect, "get_clippref_args");
    mltofx_apply_clip_preferences(image_effect, pref_args);
}

static void mltofx_set_mask_clip_disconnected(mlt_properties image_effect)
{
    if (!image_effect)
        return;

    mlt_properties mask_clip = NULL;
    mlt_properties mask_clip_props = NULL;
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Mask",
                  (OfxImageClipHandle *) &mask_clip,
                  (OfxPropertySetHandle *) &mask_clip_props);
    if (!mask_clip_props)
        return;

    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImageClipPropConnected, 0, 0);
    propSetString((OfxPropertySetHandle) mask_clip_props,
                  kOfxImageEffectPropComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) mask_clip_props,
                  kOfxImageClipPropUnmappedComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) mask_clip_props,
                  kOfxImageEffectPropPixelDepth,
                  0,
                  kOfxBitDepthNone);
    propSetString((OfxPropertySetHandle) mask_clip_props,
                  kOfxImageClipPropUnmappedPixelDepth,
                  0,
                  kOfxBitDepthNone);
    propSetDouble((OfxPropertySetHandle) mask_clip_props, kOfxImagePropPixelAspectRatio, 0, 1.0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropBounds, 0, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropBounds, 1, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropBounds, 2, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropBounds, 3, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropRegionOfDefinition, 0, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropRegionOfDefinition, 1, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropRegionOfDefinition, 2, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImagePropRegionOfDefinition, 3, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props, kOfxImageEffectPropRenderQualityDraft, 0, 0);
    propSetInt((OfxPropertySetHandle) mask_clip_props,
               "uk.co.thefoundry.OfxImageEffectPropView",
               0,
               0);
}

const void *MltOfxfetchSuite(OfxPropertySetHandle host, const char *suiteName, int suiteVersion)
{
    mlt_log_debug(NULL, "[openfx] fetchSuite: `%s` v%d\n", suiteName, suiteVersion);
    if (strcmp(suiteName, kOfxImageEffectSuite) == 0 && suiteVersion == 1) {
        return &MltOfxImageEffectSuiteV1;
    } else if (strcmp(suiteName, kOfxParameterSuite) == 0 && suiteVersion == 1) {
        return &MltOfxParameterSuiteV1;
    } else if (strcmp(suiteName, kOfxPropertySuite) == 0 && suiteVersion == 1) {
        return &MltOfxPropertySuiteV1;
    } else if (strcmp(suiteName, kOfxMemorySuite) == 0 && suiteVersion == 1) {
        return &MltOfxMemorySuiteV1;
    } else if (strcmp(suiteName, kOfxMultiThreadSuite) == 0 && suiteVersion == 1) {
        return &MltOfxMultiThreadSuiteV1;
    } else if (strcmp(suiteName, kOfxMessageSuite) == 0 && suiteVersion == 1) {
        return &MltOfxMessageSuiteV1;
    } else if (strcmp(suiteName, kOfxMessageSuite) == 0 && suiteVersion == 2) {
        return &MltOfxMessageSuiteV2;
    } else if (strcmp(suiteName, kOfxInteractSuite) == 0 && suiteVersion == 1) {
        return &MltOfxInteractSuiteV1;
    } else if (strcmp(suiteName, kOfxDrawSuite) == 0 && suiteVersion == 1) {
        // return &MltOfxDrawSuiteV1
        return NULL;
    } else if (strcmp(suiteName, kOfxProgressSuite) == 0 && suiteVersion == 1) {
        return &MltOfxProgressSuiteV1;
    } else if (strcmp(suiteName, kOfxProgressSuite) == 0 && suiteVersion == 2) {
        return &MltOfxProgressSuiteV2;
    } else if (strcmp(suiteName, kOfxTimeLineSuite) == 0 && suiteVersion == 1) {
        return &MltOfxTimeLineSuiteV1;
    } else if (strcmp(suiteName, kOfxParametricParameterSuite) == 0 && suiteVersion == 1) {
        return &MltOfxParametricParameterSuiteV1;
    } else if (strcmp(suiteName, kOfxOpenGLRenderSuite) == 0 && suiteVersion == 1) {
        // return &MltOfxOpenGLRenderSuiteV1
        return NULL;
    }

    mlt_log_debug(NULL, "[openfx] %s v%d is not supported\n", suiteName, suiteVersion);
    return NULL;
}

OfxHost MltOfxHost = {NULL, MltOfxfetchSuite};

void mltofx_init_host_properties(OfxPropertySetHandle host_properties)
{
    mlt_log_debug(NULL, "[openfx] Initializing OpenFX host properties\n");
    // Core host identity and API version
    propSetString(host_properties, kOfxPropName, 0, "MLT");
    propSetString(host_properties, kOfxPropLabel, 0, "MLT");
    propSetString(host_properties, kOfxPropVersionLabel, 0, LIBMLT_VERSION);
    propSetInt(host_properties, kOfxPropVersion, 0, LIBMLT_VERSION_MAJOR);
    propSetInt(host_properties, kOfxPropVersion, 1, LIBMLT_VERSION_MINOR);
    propSetInt(host_properties, kOfxPropVersion, 2, LIBMLT_VERSION_REVISION);
    propSetInt(host_properties, kOfxPropAPIVersion, 0, 1);
    propSetInt(host_properties, kOfxPropAPIVersion, 1, 5);

    // Image effect context and supported pixel formats
    propSetString(host_properties, kOfxImageEffectPropContext, 0, kOfxImageEffectContextFilter);
    propSetString(host_properties, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
    propSetString(host_properties, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
    propSetString(host_properties, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthHalf);
    propSetString(host_properties, kOfxImageEffectPropSupportedPixelDepths, 3, kOfxBitDepthFloat);
    propSetString(host_properties,
                  kOfxImageEffectPropSupportedComponents,
                  0,
                  kOfxImageComponentRGBA);
    propSetString(host_properties, kOfxImageEffectPropSupportedComponents, 1, kOfxImageComponentRGB);

    // Image effect host capabilities
    propSetInt(host_properties, kOfxImageEffectHostPropIsBackground, 0, 1);
    propSetInt(host_properties, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSupportsMultipleClipPARs, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSupportsMultiResolution, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSupportsTiles, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropTemporalClipAccess, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSetableFrameRate, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSetableFielding, 0, 0);
    propSetInt(host_properties, kOfxImageEffectInstancePropSequentialRender, 0, 0);
    propSetInt(host_properties, kOfxImageEffectPropSupportsOverlays, 0, 0);
    propSetString(host_properties,
                  kOfxImageEffectHostPropNativeOrigin,
                  0,
                  kOfxHostNativeOriginBottomLeft);

    // GPU/accelerated rendering — not supported; CPU rendering is supported
    propSetString(host_properties, kOfxImageEffectPropCPURenderSupported, 0, "true");
    propSetString(host_properties, kOfxImageEffectPropOpenGLRenderSupported, 0, "false");
    propSetString(host_properties, kOfxImageEffectPropCudaRenderSupported, 0, "false");
    propSetString(host_properties, kOfxImageEffectPropCudaStreamSupported, 0, "false");
    propSetString(host_properties, kOfxImageEffectPropMetalRenderSupported, 0, "false");
    propSetString(host_properties, kOfxImageEffectPropOpenCLRenderSupported, 0, "false");
    propSetString(host_properties, kOfxImageEffectPropOpenCLSupported, 0, "false");

    // Parameter host capabilities
    propSetInt(host_properties, kOfxParamHostPropSupportsCustomAnimation, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsStringAnimation, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsBooleanAnimation, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsChoiceAnimation, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsStrChoiceAnimation, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsStrChoice, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsCustomInteract, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropMaxParameters, 0, -1);
    propSetInt(host_properties, kOfxParamHostPropMaxPages, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropPageRowColumnCount, 0, 0);
    propSetInt(host_properties, kOfxParamHostPropPageRowColumnCount, 1, 0);
    propSetInt(host_properties, kOfxParamHostPropSupportsParametricAnimation, 0, 0);
}

void mltofx_create_instance(OfxPlugin *plugin, mlt_properties image_effect)
{
    OfxStatus status_code = plugin->mainEntry(kOfxActionCreateInstance,
                                              (OfxImageEffectHandle) image_effect,
                                              NULL,
                                              NULL);
    mltofx_log_status_code(status_code, kOfxActionCreateInstance);
}

void mltofx_destroy_instance(OfxPlugin *plugin, mlt_properties image_effect)
{
    OfxStatus status_code = plugin->mainEntry(kOfxActionDestroyInstance,
                                              (OfxImageEffectHandle) image_effect,
                                              NULL,
                                              NULL);
    mltofx_log_status_code(status_code, kOfxActionDestroyInstance);
}

void mltofx_set_render_scale(mlt_properties image_effect, double scale_x, double scale_y)
{
    if (!image_effect)
        return;
    if (scale_x <= 0.0)
        scale_x = 1.0;
    if (scale_y <= 0.0)
        scale_y = 1.0;
    mlt_properties_set_double(image_effect, "_render_scale_x", scale_x);
    mlt_properties_set_double(image_effect, "_render_scale_y", scale_y);
}

static void mltofx_get_render_scale(mlt_properties image_effect, double *scale_x, double *scale_y)
{
    if (scale_x)
        *scale_x = 1.0;
    if (scale_y)
        *scale_y = 1.0;
    if (!image_effect)
        return;

    double x = mlt_properties_get_double(image_effect, "_render_scale_x");
    double y = mlt_properties_get_double(image_effect, "_render_scale_y");
    if (x > 0.0 && scale_x)
        *scale_x = x;
    if (y > 0.0 && scale_y)
        *scale_y = y;
}

void mltofx_set_project_properties(mlt_properties image_effect,
                                   int width,
                                   int height,
                                   double pixel_aspect_ratio,
                                   double fps,
                                   double duration)
{
    if (!image_effect)
        return;
    if (pixel_aspect_ratio <= 0.0)
        pixel_aspect_ratio = 1.0;
    // OFX canonical coordinates: x scaled by PAR, y in pixels.
    double canonical_width = (double) width * pixel_aspect_ratio;
    double canonical_height = (double) height;
    propSetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectExtent,
                  0,
                  canonical_width);
    propSetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectExtent,
                  1,
                  canonical_height);
    propSetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectSize,
                  0,
                  canonical_width);
    propSetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectSize,
                  1,
                  canonical_height);
    propSetDouble((OfxPropertySetHandle) image_effect, kOfxImageEffectPropProjectOffset, 0, 0.0);
    propSetDouble((OfxPropertySetHandle) image_effect, kOfxImageEffectPropProjectOffset, 1, 0.0);
    propSetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectPixelAspectRatio,
                  0,
                  pixel_aspect_ratio);
    if (fps > 0.0)
        propSetDouble((OfxPropertySetHandle) image_effect, kOfxImageEffectPropFrameRate, 0, fps);
    if (duration > 0.0)
        propSetDouble((OfxPropertySetHandle) image_effect,
                      kOfxImageEffectInstancePropEffectDuration,
                      0,
                      duration);
}

int mltofx_allows_frame_threading(mlt_properties image_effect)
{
    if (!image_effect)
        return 0;

    mlt_properties props = mlt_properties_get_properties(image_effect, "props");
    if (!props)
        return 0;

    char *thread_safety = NULL;
    if (propGetString((OfxPropertySetHandle) props,
                      kOfxImageEffectPluginRenderThreadSafety,
                      0,
                      &thread_safety)
            != kOfxStatOK
        || !thread_safety) {
        // OFX default is instance-safe when not provided.
        return 1;
    }

    return strcmp(thread_safety, kOfxImageEffectRenderUnsafe) != 0;
}

void mltofx_set_source_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format,
                                 double pixel_aspect_ratio,
                                 const char *ofx_depth,
                                 int top_left_origin)
{
    mlt_properties clip;
    mlt_properties clip_prop;
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);

    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Source",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_prop);

    int depth_byte_size = 4;
    const char *depth_format = kOfxBitDepthByte;
    const char *prop_component = kOfxImageComponentRGBA;
    if (ofx_depth) {
        depth_format = ofx_depth;
        if (!strcmp(ofx_depth, kOfxBitDepthFloat))
            depth_byte_size = 16;
        else if (!strcmp(ofx_depth, kOfxBitDepthShort) || !strcmp(ofx_depth, kOfxBitDepthHalf))
            depth_byte_size = 8;
        else
            depth_byte_size = 4;
    } else if (format == mlt_image_rgba64) {
        depth_byte_size = 8;
        depth_format = kOfxBitDepthShort;
    } else if (format == mlt_image_rgb) {
        depth_byte_size = 3;
        prop_component = kOfxImageComponentRGB;
    }
    mlt_log_debug(NULL,
                  "[openfx] %s: format=%s, depth=%s\n",
                  __FUNCTION__,
                  format == mlt_image_rgba64
                      ? "rgba64"
                      : (format == mlt_image_rgba ? "rgba"
                                                  : (format == mlt_image_rgb ? "rgb" : "Unknown")),
                  depth_format);

    int row_bytes = width * depth_byte_size;
    uint8_t *image_origin = image;
    int top_left_compat = mltofx_source_prefers_top_left_origin(plugin, depth_format);
    if (!top_left_compat && !top_left_origin && row_bytes > 0 && height > 0) {
        // OFX CPU images are addressed from lower-left. Point data at the
        // first byte of the last scanline and use negative row bytes.
        image_origin = image + ((size_t) (height - 1) * (size_t) row_bytes);
        row_bytes = -row_bytes;
    }

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRowBytes, 0, row_bytes);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPixelDepth, 0, depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedPixelDepth,
                  0,
                  depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropComponents,
                  0,
                  prop_component);
    propSetPointer((OfxPropertySetHandle) clip_prop, kOfxImagePropData, 0, (void *) image_origin);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 0, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 1, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 2, width);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 3, height);

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 0, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 1, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 2, width);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 3, height);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  1,
                  kOfxImageComponentRGBA);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  2,
                  kOfxImageComponentRGB);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  3,
                  kOfxImageComponentAlpha);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  1,
                  kOfxImageComponentRGBA);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  2,
                  kOfxImageComponentRGB);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  3,
                  kOfxImageComponentAlpha);

    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImagePropPixelAspectRatio,
                  0,
                  pixel_aspect_ratio);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropPreMultiplication,
                  0,
                  kOfxImageUnPreMultiplied);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImagePropField, 0, kOfxImageFieldBoth);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropFieldOrder,
                  0,
                  kOfxImageFieldBoth);

    propSetString((OfxPropertySetHandle) clip_prop,
                  "OfxImageEffectPropRenderQuality",
                  0,
                  "OfxImageEffectPropRenderQualityBest");

    char *tstr = calloc(1, strlen("Source") + 11);
    sprintf(tstr, "%s%04d%04d", "Source", rand() % 9999, rand() % 9999);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImagePropUniqueIdentifier, 0, tstr);
    free(tstr);

    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImageClipPropConnected, 0, 1);

    mltofx_set_mask_clip_disconnected(image_effect);

    // Preserve plugin-selected clip preferences across clip data refreshes.
    mltofx_apply_cached_clip_preferences(image_effect);
}

void mltofx_set_output_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format,
                                 double pixel_aspect_ratio,
                                 const char *ofx_depth,
                                 int top_left_origin)
{
    mlt_properties clip;
    mlt_properties clip_prop;
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);

    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_prop);

    int depth_byte_size = 4;
    const char *depth_format = kOfxBitDepthByte;
    const char *prop_component = kOfxImageComponentRGBA;
    if (ofx_depth) {
        depth_format = ofx_depth;
        if (!strcmp(ofx_depth, kOfxBitDepthFloat))
            depth_byte_size = 16;
        else if (!strcmp(ofx_depth, kOfxBitDepthShort) || !strcmp(ofx_depth, kOfxBitDepthHalf))
            depth_byte_size = 8;
        else
            depth_byte_size = 4;
    } else if (format == mlt_image_rgba64) {
        depth_byte_size = 8;
        depth_format = kOfxBitDepthShort;
    } else if (format == mlt_image_rgb) {
        depth_byte_size = 3;
        prop_component = kOfxImageComponentRGB;
    }

    int row_bytes = width * depth_byte_size;
    uint8_t *image_origin = image;
    if (!top_left_origin && row_bytes > 0 && height > 0) {
        // OFX CPU images are addressed from lower-left. Point data at the
        // first byte of the last scanline and use negative row bytes.
        image_origin = image + ((size_t) (height - 1) * (size_t) row_bytes);
        row_bytes = -row_bytes;
    }

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRowBytes, 0, row_bytes);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPixelDepth, 0, depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedPixelDepth,
                  0,
                  depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropComponents,
                  0,
                  prop_component);
    propSetPointer((OfxPropertySetHandle) clip_prop, kOfxImagePropData, 0, (void *) image_origin);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 0, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 1, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 2, width);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropBounds, 3, height);

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 0, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 1, 0);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 2, width);
    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 3, height);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  1,
                  kOfxImageComponentRGBA);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  2,
                  kOfxImageComponentRGB);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropSupportedComponents,
                  3,
                  kOfxImageComponentAlpha);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  0,
                  kOfxImageComponentNone);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  1,
                  kOfxImageComponentRGBA);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  2,
                  kOfxImageComponentRGB);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedComponents,
                  3,
                  kOfxImageComponentAlpha);

    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropPreMultiplication,
                  0,
                  kOfxImageUnPreMultiplied);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImagePropField, 0, kOfxImageFieldBoth);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropFieldOrder,
                  0,
                  kOfxImageFieldBoth);

    propSetString((OfxPropertySetHandle) clip_prop,
                  "OfxImageEffectPropRenderQuality",
                  0,
                  "OfxImageEffectPropRenderQualityBest");

    char *tstr = calloc(1, strlen("Output") + 11);
    sprintf(tstr, "%s%04d%04d", "Output", rand() % 9999, rand() % 9999);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImagePropUniqueIdentifier, 0, tstr);
    free(tstr);

    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImagePropPixelAspectRatio,
                  0,
                  pixel_aspect_ratio);

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImageClipPropConnected, 0, 1);

    mltofx_set_mask_clip_disconnected(image_effect);

    // Preserve plugin-selected clip preferences across clip data refreshes.
    mltofx_apply_cached_clip_preferences(image_effect);
}

int mltofx_detect_plugin(OfxPlugin *plugin)
{
    mlt_properties image_effect = mlt_properties_new();
    mlt_properties clips = mlt_properties_new();
    mlt_properties props = mlt_properties_new();
    mlt_properties iparams = mlt_properties_new();
    mlt_properties params = mlt_properties_new();

    mlt_properties_set_properties(image_effect, "clips", clips);
    mlt_properties_set_properties(image_effect, "props", props);
    mlt_properties_set_properties(image_effect, "params", iparams);
    mlt_properties_set_properties(iparams, "plugin_props", props);
    mlt_properties_set_properties(image_effect, "mltofx_params", params);

    propSetString((OfxPropertySetHandle) props,
                  kOfxImageEffectPropContext,
                  0,
                  kOfxImageEffectContextFilter);

    plugin->setHost(&MltOfxHost);

    OfxStatus status_code = kOfxStatErrUnknown;
    status_code = plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);
    mltofx_log_status_code(status_code, "kOfxActionLoad");

    if (status_code != kOfxStatOK) {
        return 0;
    }

    status_code
        = plugin->mainEntry(kOfxActionDescribe, (OfxImageEffectHandle) image_effect, NULL, NULL);
    mltofx_log_status_code(status_code, "kOfxActionDescribe");

    if (status_code != kOfxStatOK) {
        plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL);
        return 0;
    }

    status_code = plugin->mainEntry(kOfxImageEffectActionDescribeInContext,
                                    (OfxImageEffectHandle) image_effect,
                                    (OfxPropertySetHandle) MltOfxHost.host,
                                    NULL);

    int describe_in_context_valid = 0;
    mltofx_log_status_code(status_code, "kOfxImageEffectActionDescribeInContext");
    if (status_code != kOfxStatErrMissingHostFeature) {
        plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL);
        describe_in_context_valid = 1;
    }

    int i = 0;
    int count = 0;

    propGetDimension((OfxPropertySetHandle) props, kOfxImageEffectPropSupportedContexts, &count);
    for (i = 0; i < count; i++) {
        char *context;
        propGetString((OfxPropertySetHandle) props,
                      kOfxImageEffectPropSupportedContexts,
                      i,
                      &context);
        if (!strcmp(context, kOfxImageEffectContextFilter)) {
            break;
        }
    }
    if (i == count) {
        mlt_log_debug(NULL, "[openfx] Plugin not a filter: %s\n", plugin->pluginIdentifier);
        // since plugin is not filter then load fail so we must not unload it
        return 0;
    }

    count = 0;

    propGetDimension((OfxPropertySetHandle) props, kOfxImageEffectPropSupportedPixelDepths, &count);
    for (i = 0; i < count; i++) {
        char *depth;
        propGetString((OfxPropertySetHandle) props,
                      kOfxImageEffectPropSupportedPixelDepths,
                      i,
                      &depth);
        if (!strcmp(depth, kOfxBitDepthByte) || !strcmp(depth, kOfxBitDepthShort)
            || !strcmp(depth, kOfxBitDepthHalf) || !strcmp(depth, kOfxBitDepthFloat)) {
            break;
        }
    }
    if (i == count) {
        mlt_log_verbose(NULL,
                        "[openfx] Plugin does not support byte, short, half, or float pixels: %s\n",
                        plugin->pluginIdentifier);
        // since no pixel depth is supported by us then plugin is load fail so we must not unload it
        return 0;
    }

    if (describe_in_context_valid)
        return 1;

    plugin->mainEntry(kOfxActionUnload, NULL, NULL, NULL);
    mlt_properties_close(image_effect);
    mlt_properties_close(clips);
    mlt_properties_close(iparams);
    mlt_properties_close(props);
    mlt_properties_close(params);
    return 0;
}

static int param_type_is_supported(const char *type)
{
    return strcmp(type, kOfxParamTypeCustom) && strcmp(type, kOfxParamTypeBytes)
           && strcmp(type, kOfxParamTypePage) && strcmp(type, kOfxParamTypePushButton);
}

void *mltofx_fetch_params(OfxPlugin *plugin, mlt_properties params, mlt_properties mlt_metadata)
{
    mlt_properties image_effect = mlt_properties_new();
    mlt_properties clips = mlt_properties_new();
    mlt_properties props = mlt_properties_new();
    mlt_properties iparams = mlt_properties_new();

    mlt_properties_set_properties(image_effect, "clips", clips);
    mlt_properties_set_properties(image_effect, "props", props);
    mlt_properties_set_properties(image_effect, "params", iparams);
    mlt_properties_set_properties(iparams, "plugin_props", props);
    mlt_properties_set_properties(image_effect, "mltofx_params", params);

    propSetString((OfxPropertySetHandle) props,
                  kOfxImageEffectPropContext,
                  0,
                  kOfxImageEffectContextFilter);

    plugin->setHost(&MltOfxHost);
    OfxStatus status_code = kOfxStatErrUnknown;
    status_code = plugin->mainEntry(kOfxActionLoad, NULL, NULL, NULL);
    mltofx_log_status_code(status_code, "kOfxActionLoad");
    status_code
        = plugin->mainEntry(kOfxActionDescribe, (OfxImageEffectHandle) image_effect, NULL, NULL);
    mltofx_log_status_code(status_code, "kOfxActionDescribe");

    status_code = plugin->mainEntry(kOfxImageEffectActionDescribeInContext,
                                    (OfxImageEffectHandle) image_effect,
                                    (OfxPropertySetHandle) MltOfxHost.host,
                                    NULL);

    // some plugins need to set some attributes at Source and Output before any other operation happen
    mlt_properties clip = NULL, clip_props = NULL;
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Source",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_props);

    if (clip_props != NULL) {
        propSetString((OfxPropertySetHandle) clip_props,
                      kOfxImageClipPropFieldOrder,
                      0,
                      kOfxImageFieldNone);

        propSetString((OfxPropertySetHandle) clip_props,
                      "OfxImageEffectPropRenderQuality",
                      0,
                      "OfxImageEffectPropRenderQualityBest");

        propSetString((OfxPropertySetHandle) clip_props, kOfxImagePropField, 0, kOfxImageFieldNone);
    }

    mltofx_set_mask_clip_disconnected(image_effect);

    clip = NULL, clip_props = NULL;
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_props);

    if (clip_props != NULL) {
        propSetString((OfxPropertySetHandle) clip_props,
                      kOfxImageClipPropFieldOrder,
                      0,
                      kOfxImageFieldNone);

        propSetString((OfxPropertySetHandle) clip_props,
                      "OfxImageEffectPropRenderQuality",
                      0,
                      "OfxImageEffectPropRenderQualityBest");

        propSetString((OfxPropertySetHandle) clip_props, kOfxImagePropField, 0, kOfxImageFieldNone);
    }

    mltofx_log_status_code(status_code, "kOfxImageEffectActionDescribeInContext");

    if (mlt_metadata != NULL) {
        char *str_value = "";
        propGetString((OfxPropertySetHandle) props, kOfxPropPluginDescription, 0, &str_value);
        if (str_value[0] != '\0') {
            mlt_properties_set(mlt_metadata, "description", str_value);
        }
        str_value = "";
        propGetString((OfxPropertySetHandle) props, kOfxPropLongLabel, 0, &str_value);
        if (str_value[0] == '\0')
            propGetString((OfxPropertySetHandle) props, kOfxPropLabel, 0, &str_value);
        if (str_value[0] != '\0') {
            mlt_properties_set(mlt_metadata, "title", str_value);
        }
        str_value = "";
        propGetString((OfxPropertySetHandle) props, kOfxPropVersionLabel, 0, &str_value);
        if (str_value[0] != '\0') {
            mlt_properties_set(mlt_metadata, "version", str_value);
        } else {
            // Build version string from the integer array (e.g. "1.2.3")
            char version_str[64] = "";
            int dim = 0;
            propGetDimension((OfxPropertySetHandle) props, kOfxPropVersion, &dim);
            for (int vi = 0; vi < dim; ++vi) {
                int v = 0;
                propGetInt((OfxPropertySetHandle) props, kOfxPropVersion, vi, &v);
                char part[16];
                snprintf(part, sizeof(part), vi == 0 ? "%d" : ".%d", v);
                strncat(version_str, part, sizeof(version_str) - strlen(version_str) - 1);
            }
            if (version_str[0] != '\0')
                mlt_properties_set(mlt_metadata, "version", version_str);
        }
    }

    int iparams_length = mlt_properties_count(iparams);
    int mlt_param_count = 0;
    // starting with 1 to skip the plugin_props which is not a param but just a reference to the plugin properties
    for (int ipiter = 1; ipiter < iparams_length; ++ipiter) {
        char *name = mlt_properties_get_name(iparams, ipiter);
        mlt_properties pp = mlt_properties_get_properties(iparams, name);
        mlt_properties ppp = mlt_properties_get_properties(pp, "p");

        // Skip parameters marked secret.
        int is_secret = -1;
        propGetInt((OfxPropertySetHandle) ppp, kOfxParamPropSecret, 0, &is_secret);
        if (is_secret == 1)
            continue;

        // Skip unsupported parameter types.
        char *param_type = mlt_properties_get(pp, "t");
        if (!param_type_is_supported(param_type))
            continue;

        // Create a new MLT property set for this parameter and add it to the params list
        // in accordance with the metadata schema.
        mlt_properties p = mlt_properties_new();
        char key[20];
        snprintf(key, sizeof(key), "%d", mlt_param_count++);
        mlt_properties_set_data(params, key, p, 0, (mlt_destructor) mlt_properties_close, NULL);

        mlt_properties_set(p, "identifier", name);

        char *label = NULL;
        propGetString((OfxPropertySetHandle) ppp, kOfxPropLabel, 0, &label);
        if (label)
            mlt_properties_set(p, "title", label);

        char *hint = NULL;
        propGetString((OfxPropertySetHandle) ppp, kOfxParamPropHint, 0, &hint);
        if (hint)
            mlt_properties_set(p, "description", hint);

        if (strcmp(param_type, kOfxParamTypeInteger) == 0) {
            mlt_properties_set(p, "type", "integer");
        } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
            mlt_properties_set(p, "type", "float");
        } else if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
            mlt_properties_set(
                p,
                "type",
                "string"); // setting this to string let kdenlive and other MLT users render this as combobox dropdown list
        } else if (strcmp(param_type, kOfxParamTypeString) == 0) {
            mlt_properties_set(p, "type", "string");
        } else if (strcmp(param_type, kOfxParamTypeBoolean) == 0) {
            mlt_properties_set(p, "type", "boolean");
        } else if (strcmp(param_type, kOfxParamTypeGroup) == 0) {
            mlt_properties_set(p, "type", "group");
        } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0
                   || strcmp(param_type, kOfxParamTypeRGB) == 0) {
            mlt_properties_set(p, "type", "color");
            mlt_properties_set(p, "widget", "color");
        } else if (strcmp(param_type, kOfxParamTypeDouble2D)
                   == 0) { // can be rendered as 2 double number input fields
            mlt_properties_set(p, "type", "float");

            char *type = kOfxParamDoubleTypeXY;
            char *coordinate_system = kOfxParamCoordinatesCanonical;
            propGetString((OfxPropertySetHandle) ppp, kOfxParamPropDoubleType, 0, &type);
            propGetString((OfxPropertySetHandle) ppp,
                          kOfxParamPropDefaultCoordinateSystem,
                          0,
                          &coordinate_system);

            if (strcmp(type, kOfxParamDoubleTypeXYAbsolute) == 0) {
                mlt_properties_set(p, "widget", "point");
            } else if (strcmp(type, kOfxParamDoubleTypeXY) == 0) {
                mlt_properties_set(p, "widget", "size");
            }

            if (strcmp(coordinate_system, kOfxParamCoordinatesCanonical) == 0) {
                mlt_properties_set(p, "normalized_coordinates", "no");
            } else if (strcmp(coordinate_system, kOfxParamCoordinatesNormalised) == 0) {
                mlt_properties_set(p, "normalized_coordinates", "yes");
            }
        } else if (strcmp(param_type, kOfxParamTypeInteger2D) == 0) {
            // can be rendered as 2 integer number input fields
            mlt_properties_set(p, "type", "integer");
        }

        if (strcmp(param_type, kOfxParamTypeGroup) != 0) {
            int animation = 1;
            propGetInt((OfxPropertySetHandle) ppp, kOfxParamPropAnimates, 0, &animation);
            mlt_properties_set(p, "animation", animation ? "yes" : "no");
        }
        mlt_properties_set(p, "mutable", "yes");

        // Iterate through the properties of the first dimension to find all the params.
        mlt_properties dim1 = mlt_properties_get_properties(ppp, "0");
        int min_set = 0;
        int max_set = 0;
        int p_length = mlt_properties_count(dim1);
        for (int jt = 0; jt < p_length; ++jt) {
            char *p_name = mlt_properties_get_name(dim1, jt);
            if (strcmp(p_name, kOfxParamPropDefault) == 0) {
                if (strcmp(param_type, kOfxParamTypeInteger) == 0
                    || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
                    int default_value = 0;
                    propGetInt((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
                    mlt_properties_set_int(p, "default", default_value);
                } else if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
                    int default_value = 0;
                    int choice_count = 0;
                    char *choice_str = NULL;

                    propGetInt((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
                    propGetDimension((OfxPropertySetHandle) ppp,
                                     kOfxParamPropChoiceOption,
                                     &choice_count);

                    if (default_value >= 0 && default_value < choice_count
                        && propGetString((OfxPropertySetHandle) ppp,
                                         kOfxParamPropChoiceOption,
                                         default_value,
                                         &choice_str)
                               == kOfxStatOK
                        && choice_str) {
                        mlt_properties_set(p, "default", choice_str);
                    }
                } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
                    double default_value = 0.0;
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
                    mlt_properties_set_double(p, "default", default_value);
                } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
                    double default_value1 = 0.0, default_value2 = 0.0;

                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &default_value1);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 1, &default_value2);

                    // for some reason with DBL_MIN DBL_MAX it segfault
                    if (!isnormal(default_value1) || default_value1 <= FLT_MIN
                        || default_value1 >= FLT_MAX)
                        default_value1 = 0.0;
                    if (!isnormal(default_value2) || default_value2 <= FLT_MIN
                        || default_value2 >= FLT_MAX)
                        default_value2 = 0.0;

                    char default_value[90] = "";
                    snprintf(default_value,
                             sizeof(default_value),
                             "%.4f %.4f",
                             default_value1,
                             default_value2);
                    mlt_properties_set(p, "default", default_value);

                } else if (strcmp(param_type, kOfxParamTypeString) == 0) {
                    char *default_value = "";
                    propGetString((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
                    mlt_properties_set(p, "default", default_value);
                } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
                    double r, g, b, a;
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &r);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 1, &g);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 2, &b);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 3, &a);

                    char default_value[10] = "#000000FF";
                    snprintf(default_value,
                             sizeof(default_value),
                             "#%02X%02X%02X%02X",
                             (unsigned char) (a * 255.0),
                             (unsigned char) (r * 255.0),
                             (unsigned char) (g * 255.0),
                             (unsigned char) (b * 255.0));

                    mlt_properties_set(p, "default", default_value);
                } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
                    double r, g, b;
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &r);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 1, &g);
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 2, &b);

                    char default_value[10] = "#000000FF";
                    snprintf(default_value,
                             sizeof(default_value),
                             "#%02X%02X%02XFF",
                             (unsigned char) (r * 255.0),
                             (unsigned char) (g * 255.0),
                             (unsigned char) (b * 255.0));

                    mlt_properties_set(p, "default", default_value);
                }

                if (strcmp(param_type, kOfxParamTypeChoice) == 0) {
                    char key[20];
                    int count = 0;
                    mlt_properties choices = mlt_properties_new();
                    mlt_properties_set_data(p,
                                            "values",
                                            choices,
                                            0,
                                            (mlt_destructor) mlt_properties_close,
                                            NULL);

                    propGetDimension((OfxPropertySetHandle) ppp, kOfxParamPropChoiceOption, &count);

                    for (int jtr = 0; jtr < count; ++jtr) {
                        key[0] = '\0';
                        snprintf(key, sizeof(key), "%d", jtr);
                        char *choice_str = NULL;
                        propGetString((OfxPropertySetHandle) ppp,
                                      kOfxParamPropChoiceOption,
                                      jtr,
                                      &choice_str);
                        mlt_properties_set(choices, key, choice_str);
                    }
                }
            } else if ((strcmp(p_name, kOfxParamPropMin) == 0 && min_set == 0)
                       || strcmp(p_name, kOfxParamPropDisplayMin) == 0) {
                if (strcmp(param_type, kOfxParamTypeInteger) == 0
                    || strcmp(param_type, kOfxParamTypeChoice) == 0
                    || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
                    min_set = 1;
                    int minimum_value = 0;
                    propGetInt((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
                    mlt_properties_set_int(p, "minimum", minimum_value);
                } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
                    double minimum_value = 0.0;
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
                    mlt_properties_set_double(p, "minimum", minimum_value);
                } else if (strcmp(param_type, kOfxParamTypeString) == 0) {
                    char *minimum_value = "";
                    propGetString((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
                    mlt_properties_set(p, "minimum", minimum_value);
                }
            } else if ((strcmp(p_name, kOfxParamPropMax) == 0 && max_set == 0)
                       || strcmp(p_name, kOfxParamPropDisplayMax) == 0) {
                if (strcmp(param_type, kOfxParamTypeInteger) == 0
                    || strcmp(param_type, kOfxParamTypeChoice) == 0
                    || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
                    max_set = 1;
                    int maximum_value = 0;
                    propGetInt((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
                    mlt_properties_set_int(p, "maximum", maximum_value);
                } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
                    double maximum_value = 0.0;
                    propGetDouble((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
                    mlt_properties_set_double(p, "maximum", maximum_value);
                } else if (strcmp(param_type, kOfxParamTypeString) == 0) {
                    char *maximum_value = "";
                    propGetString((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
                    mlt_properties_set(p, "maximum", maximum_value);
                }
            } else if (strcmp(p_name, kOfxParamPropStringMode) == 0) {
                char *str_value = "";
                propGetString((OfxPropertySetHandle) ppp, p_name, 0, &str_value);
                if (strcmp(str_value, kOfxParamStringIsLabel) == 0) {
                    mlt_properties_set(p, "readonly", "yes");
                }
            } else if (strcmp(p_name, kOfxParamPropParent) == 0) {
                char *str_value = "";
                propGetString((OfxPropertySetHandle) ppp, p_name, 0, &str_value);
                mlt_properties_set(p, "group", str_value);
            }
        }
    }

    // Seed runtime param values from OFX defaults so plugins read stable values
    // even when the user has not overridden a parameter.
    mltofx_initialize_param_values(iparams);

    mlt_properties_close(clips);
    mlt_properties_close(iparams);
    mlt_properties_close(props);
    return image_effect;
}

void mltofx_param_set_value(mlt_properties params, char *key, mltofx_property_type type, ...)
{
    mlt_properties param = NULL;
    paramGetHandle((OfxParamSetHandle) params, key, (OfxParamHandle *) &param, NULL);
    if (!param)
        return;
    mlt_properties param_props = mlt_properties_get_properties(param, "p");
    if (!param_props)
        return;

    va_list ap;
    va_start(ap, type);

    switch (type) {
    case mltofx_prop_int: {
        int value = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } break;

    case mltofx_prop_double: {
        double value = va_arg(ap, double);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } break;

    case mltofx_prop_string: {
        char *value = va_arg(ap, char *);
        propSetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } break;

    case mltofx_prop_color: {
        mlt_color value = va_arg(ap, mlt_color);
        double red = (double) value.r / 255.0;
        double green = (double) value.g / 255.0;
        double blue = (double) value.b / 255.0;
        double alpha = (double) value.a / 255.0;
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, alpha);
    } break;

    case mltofx_prop_double2d: {
        mlt_rect value = va_arg(ap, mlt_rect);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value.x);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, value.y);
    } break;

    case mltofx_prop_int2d: {
        int x = va_arg(ap, int);
        int y = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, x);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, y);
    } break;

    default:
        break;
    }

    va_end(ap);
}

void mltofx_get_clip_preferences(OfxPlugin *plugin, mlt_properties image_effect)
{
    mlt_properties get_clippref_args = mlt_properties_get_properties(image_effect,
                                                                     "get_clippref_args");

    mltofx_prime_clip_preferences(image_effect, get_clippref_args);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetClipPreferences,
                                              (OfxImageEffectHandle) image_effect,
                                              NULL,
                                              (OfxPropertySetHandle) get_clippref_args);
    mltofx_log_status_code(status_code, kOfxImageEffectActionGetClipPreferences);

    if (status_code == kOfxStatOK || status_code == kOfxStatReplyDefault)
        mltofx_apply_clip_preferences(image_effect, get_clippref_args);
}

void mltofx_get_region_of_definition(OfxPlugin *plugin, mlt_properties image_effect, double ofx_time)
{
    mlt_properties get_rod_in_args = mlt_properties_get_properties(image_effect, "get_rod_in_args");
    mlt_properties get_rod_out_args = mlt_properties_get_properties(image_effect,
                                                                    "get_rod_out_args");

    mlt_properties output_clip = NULL;
    mlt_properties output_clip_props = NULL;
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);
    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &output_clip,
                  (OfxPropertySetHandle *) &output_clip_props);

    // Read PAR for pixel<->canonical conversion; x_canonical = x_pixel * par.
    double rod_par = 1.0;
    if (output_clip_props)
        propGetDouble((OfxPropertySetHandle) output_clip_props,
                      kOfxImagePropPixelAspectRatio,
                      0,
                      &rod_par);
    if (rod_par <= 0.0)
        rod_par = 1.0;

    int default_x1 = 0;
    int default_y1 = 0;
    int default_x2 = 0;
    int default_y2 = 0;
    if (output_clip_props) {
        // Use current output RoD as default outArgs as required by OFX.
        if (propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropRegionOfDefinition,
                       0,
                       &default_x1)
                != kOfxStatOK
            || propGetInt((OfxPropertySetHandle) output_clip_props,
                          kOfxImagePropRegionOfDefinition,
                          1,
                          &default_y1)
                   != kOfxStatOK
            || propGetInt((OfxPropertySetHandle) output_clip_props,
                          kOfxImagePropRegionOfDefinition,
                          2,
                          &default_x2)
                   != kOfxStatOK
            || propGetInt((OfxPropertySetHandle) output_clip_props,
                          kOfxImagePropRegionOfDefinition,
                          3,
                          &default_y2)
                   != kOfxStatOK) {
            propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropBounds,
                       0,
                       &default_x1);
            propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropBounds,
                       1,
                       &default_y1);
            propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropBounds,
                       2,
                       &default_x2);
            propGetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropBounds,
                       3,
                       &default_y2);
        }
    }

    propSetDouble((OfxPropertySetHandle) get_rod_in_args, kOfxPropTime, 0, ofx_time);

    propSetDouble((OfxPropertySetHandle) get_rod_in_args,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) get_rod_in_args,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    // Seed outArgs with canonical defaults (x * PAR).
    propSetDouble((OfxPropertySetHandle) get_rod_out_args,
                  kOfxImageEffectPropRegionOfDefinition,
                  0,
                  (double) default_x1 * rod_par);
    propSetDouble((OfxPropertySetHandle) get_rod_out_args,
                  kOfxImageEffectPropRegionOfDefinition,
                  1,
                  (double) default_y1);
    propSetDouble((OfxPropertySetHandle) get_rod_out_args,
                  kOfxImageEffectPropRegionOfDefinition,
                  2,
                  (double) default_x2 * rod_par);
    propSetDouble((OfxPropertySetHandle) get_rod_out_args,
                  kOfxImageEffectPropRegionOfDefinition,
                  3,
                  (double) default_y2);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetRegionOfDefinition,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) get_rod_in_args,
                                              (OfxPropertySetHandle) get_rod_out_args);

    if (output_clip_props && (status_code == kOfxStatOK || status_code == kOfxStatReplyDefault)) {
        double rod_x1 = (double) default_x1;
        double rod_y1 = (double) default_y1;
        double rod_x2 = (double) default_x2;
        double rod_y2 = (double) default_y2;
        propGetDouble((OfxPropertySetHandle) get_rod_out_args,
                      kOfxImageEffectPropRegionOfDefinition,
                      0,
                      &rod_x1);
        propGetDouble((OfxPropertySetHandle) get_rod_out_args,
                      kOfxImageEffectPropRegionOfDefinition,
                      1,
                      &rod_y1);
        propGetDouble((OfxPropertySetHandle) get_rod_out_args,
                      kOfxImageEffectPropRegionOfDefinition,
                      2,
                      &rod_x2);
        propGetDouble((OfxPropertySetHandle) get_rod_out_args,
                      kOfxImageEffectPropRegionOfDefinition,
                      3,
                      &rod_y2);

        // Convert canonical ROD back to pixel coordinates for storage.
        int out_x1 = (int) floor(rod_x1 / rod_par);
        int out_y1 = (int) floor(rod_y1);
        int out_x2 = (int) ceil(rod_x2 / rod_par);
        int out_y2 = (int) ceil(rod_y2);

        if (out_x2 >= out_x1 && out_y2 >= out_y1) {
            propSetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropRegionOfDefinition,
                       0,
                       out_x1);
            propSetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropRegionOfDefinition,
                       1,
                       out_y1);
            propSetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropRegionOfDefinition,
                       2,
                       out_x2);
            propSetInt((OfxPropertySetHandle) output_clip_props,
                       kOfxImagePropRegionOfDefinition,
                       3,
                       out_y2);
        }
    }

    mltofx_log_status_code(status_code, kOfxImageEffectActionGetRegionOfDefinition);
}

void mltofx_get_regions_of_interest(
    OfxPlugin *plugin, mlt_properties image_effect, double ofx_time, double width, double height)
{
    mlt_properties get_roi_in_args = mlt_properties_get_properties(image_effect, "get_roi_in_args");
    mlt_properties get_roi_out_args = mlt_properties_get_properties(image_effect,
                                                                    "get_roi_out_args");
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);

    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 0, ofx_time);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 1, ofx_time);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    // ROI must be in canonical coordinates; read from project extent set earlier.
    double canonical_width = width;
    double canonical_height = height;
    propGetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectExtent,
                  0,
                  &canonical_width);
    propGetDouble((OfxPropertySetHandle) image_effect,
                  kOfxImageEffectPropProjectExtent,
                  1,
                  &canonical_height);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRegionOfInterest,
                  0,
                  0.0);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRegionOfInterest,
                  1,
                  0.0);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRegionOfInterest,
                  2,
                  canonical_width);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRegionOfInterest,
                  3,
                  canonical_height);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetRegionsOfInterest,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) get_roi_in_args,
                                              (OfxPropertySetHandle) get_roi_out_args);
    mltofx_log_status_code(status_code, kOfxImageEffectActionGetRegionsOfInterest);
}

void mltofx_begin_sequence_render(OfxPlugin *plugin, mlt_properties image_effect, double ofx_time)
{
    mlt_properties begin_sequence_props = mlt_properties_get_properties(image_effect,
                                                                        "begin_sequence_props");
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  0,
                  ofx_time);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  1,
                  ofx_time);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);

    propSetInt((OfxPropertySetHandle) begin_sequence_props, kOfxPropIsInteractive, 0, 0);

    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    propSetInt((OfxPropertySetHandle) begin_sequence_props,
               kOfxImageEffectPropSequentialRenderStatus,
               0,
               1);

    propSetInt((OfxPropertySetHandle) begin_sequence_props,
               kOfxImageEffectPropInteractiveRenderStatus,
               0,
               0);

    propSetInt((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropOpenGLEnabled, 0, 0);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionBeginSequenceRender,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) begin_sequence_props,
                                              NULL);
    mltofx_log_status_code(status_code, kOfxImageEffectActionBeginSequenceRender);
}

void mltofx_end_sequence_render(OfxPlugin *plugin, mlt_properties image_effect, double ofx_time)
{
    mlt_properties end_sequence_props = mlt_properties_get_properties(image_effect,
                                                                      "end_sequence_props");
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);
    propSetDouble((OfxPropertySetHandle) end_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  0,
                  ofx_time);

    propSetDouble((OfxPropertySetHandle) end_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  1,
                  ofx_time);

    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);

    propSetInt((OfxPropertySetHandle) end_sequence_props, kOfxPropIsInteractive, 0, 0);

    propSetDouble((OfxPropertySetHandle) end_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) end_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    propSetInt((OfxPropertySetHandle) end_sequence_props,
               kOfxImageEffectPropSequentialRenderStatus,
               0,
               1);

    propSetInt((OfxPropertySetHandle) end_sequence_props,
               kOfxImageEffectPropInteractiveRenderStatus,
               0,
               0);

    propSetInt((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropOpenGLEnabled, 0, 0);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionEndSequenceRender,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) end_sequence_props,
                                              NULL);
    mltofx_log_status_code(status_code, kOfxImageEffectActionEndSequenceRender);
}

void mltofx_action_render(
    OfxPlugin *plugin, mlt_properties image_effect, double ofx_time, int width, int height)
{
    mlt_properties render_in_args = mlt_properties_get_properties(image_effect, "render_in_args");
    double render_scale_x = 1.0;
    double render_scale_y = 1.0;
    mltofx_get_render_scale(image_effect, &render_scale_x, &render_scale_y);
    propSetDouble((OfxPropertySetHandle) render_in_args, kOfxPropTime, 0, ofx_time);

    propSetString((OfxPropertySetHandle) render_in_args,
                  kOfxImageEffectPropFieldToRender,
                  0,
                  kOfxImageFieldBoth);

    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 0, 0);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 1, 0);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 2, width);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 3, height);

    propSetDouble((OfxPropertySetHandle) render_in_args,
                  kOfxImageEffectPropRenderScale,
                  0,
                  render_scale_x);
    propSetDouble((OfxPropertySetHandle) render_in_args,
                  kOfxImageEffectPropRenderScale,
                  1,
                  render_scale_y);

    propSetInt((OfxPropertySetHandle) render_in_args,
               kOfxImageEffectPropSequentialRenderStatus,
               0,
               1);

    propSetInt((OfxPropertySetHandle) render_in_args,
               kOfxImageEffectPropInteractiveRenderStatus,
               0,
               0);

    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderQualityDraft, 0, 0);

    propSetString((OfxPropertySetHandle) render_in_args,
                  "OfxImageEffectPropRenderQuality",
                  0,
                  "OfxImageEffectPropRenderQualityBest");

    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropOpenGLEnabled, 0, 0);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionRender,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) render_in_args,
                                              NULL);

    mltofx_log_status_code(status_code, kOfxImageEffectActionRender);
}

mltofx_depths_mask mltofx_plugin_supported_depths(mlt_properties image_effect)
{
    mltofx_depths_mask mask = mltofx_depth_none;

    int count = 0;
    OfxPropertySetHandle effectProps;
    getPropertySet((OfxImageEffectHandle) image_effect, &effectProps);
    propGetDimension((OfxPropertySetHandle) effectProps,
                     kOfxImageEffectPropSupportedPixelDepths,
                     &count);

    for (int i = 0; i < count; ++i) {
        char *str = NULL;
        propGetString((OfxPropertySetHandle) effectProps,
                      kOfxImageEffectPropSupportedPixelDepths,
                      i,
                      &str);
        if (strcmp(str, kOfxBitDepthByte) == 0) {
            mask |= mltofx_depth_byte;
        } else if (strcmp(str, kOfxBitDepthShort) == 0) {
            mask |= mltofx_depth_short;
        } else if (strcmp(str, kOfxBitDepthHalf) == 0) {
            mask |= mltofx_depth_half;
        } else if (strcmp(str, kOfxBitDepthFloat) == 0) {
            mask |= mltofx_depth_float;
        }
    }

    return mask;
}

mltofx_components_mask mltofx_plugin_supported_components(mlt_properties image_effect)
{
    mltofx_components_mask mask = mltofx_components_none;

    mlt_properties set = mlt_properties_get_properties(image_effect, "clips");
    int clips_count = mlt_properties_count(set);

    for (int i = 0; i < clips_count; ++i) {
        char *clip_name = mlt_properties_get_name(set, i);

        mlt_properties clip, clip_props;
        clipGetHandle((OfxImageEffectHandle) image_effect,
                      clip_name,
                      (OfxImageClipHandle *) &clip,
                      (OfxPropertySetHandle *) &clip_props);

        int count = 0;
        propGetDimension((OfxPropertySetHandle) clip_props,
                         kOfxImageEffectPropSupportedComponents,
                         &count);

        for (int i = 0; i < count; ++i) {
            char *str = NULL;
            propGetString((OfxPropertySetHandle) clip_props,
                          kOfxImageEffectPropSupportedComponents,
                          i,
                          &str);
            if (strcmp(str, kOfxImageComponentRGB) == 0) {
                mask |= mltofx_components_rgb;
            } else if (strcmp(str, kOfxImageComponentRGBA) == 0) {
                mask |= mltofx_components_rgba;
            }
        }
    }

    return mask;
}
