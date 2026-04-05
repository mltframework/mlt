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

#include <float.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif

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

uint16_t *mltofx_rgba64_to_half(const uint16_t *src, int n_pixels)
{
    int count = n_pixels * 4;
    uint16_t *dst = malloc((size_t) count * sizeof(uint16_t));
    if (!dst)
        return NULL;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < count; ++i)
        dst[i] = f32_to_f16((float) src[i] * (1.0f / 65535.0f));
    return dst;
}

void mltofx_half_to_rgba64(const uint16_t *src, uint16_t *dst, int n_pixels)
{
    int count = n_pixels * 4;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < count; ++i) {
        float v = f16_to_f32(src[i]);
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        dst[i] = (uint16_t) (v * 65535.0f + 0.5f);
    }
}

float *mltofx_rgba64_to_float(const uint16_t *src, int n_pixels)
{
    int count = n_pixels * 4;
    float *dst = malloc((size_t) count * sizeof(float));
    if (!dst)
        return NULL;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < count; ++i)
        dst[i] = (float) src[i] * (1.0f / 65535.0f);
    return dst;
}

void mltofx_float_to_rgba64(const float *src, uint16_t *dst, int n_pixels)
{
    int count = n_pixels * 4;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < count; ++i) {
        float v = src[i];
        v = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        dst[i] = (uint16_t) (v * 65535.0f + 0.5f);
    }
}

/* OpenFX Header files https://github.com/AcademySoftwareFoundation/openfx/tree/main/include */
#include <ofxDrawSuite.h>
#include <ofxGPURender.h>
#include <ofxImageEffect.h>
#include <ofxOpenGLRender.h>
#include <ofxParametricParam.h>
#include <ofxProgress.h>
#include <ofxTimeLine.h>

static OfxStatus propGetInt(OfxPropertySetHandle properties,
                            const char *property,
                            int index,
                            int *value);
static OfxStatus propGetIntN(OfxPropertySetHandle properties,
                             const char *property,
                             int count,
                             int *value);

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
    if (!clip)
        return kOfxStatErrBadHandle;
    *imageHandle = (OfxPropertySetHandle) mlt_properties_get_properties((mlt_properties) clip,
                                                                        "props");
    if (region != NULL) {
        OfxRectI rect = {0, 0, 0, 0};
        propGetIntN((OfxPropertySetHandle) *imageHandle, kOfxImagePropBounds, 4, &rect.x1);
        OfxRectD *region2 = (OfxRectD *) region;
        region2->x1 = (double) rect.x1;
        region2->x2 = (double) rect.x2;
        region2->y1 = (double) rect.y1;
        region2->y2 = (double) rect.y2;
    }
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
    bounds->x1 = (double) x1;
    bounds->y1 = (double) y1;
    bounds->x2 = (double) x2;
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

static OfxStatus paramGetValueImpl(OfxParamHandle paramHandle, va_list ap)
{
    mlt_properties param = (mlt_properties) paramHandle;
    char *param_type = mlt_properties_get(param, "t");
    mlt_properties param_props = mlt_properties_get_properties(param, "p");

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0
        || strcmp(param_type, kOfxParamTypeChoice) == 0) {
        int *value = va_arg(ap, int *);
        OfxStatus status
            = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
        if (status != kOfxStatOK) {
            status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
            if (status != kOfxStatOK)
                return kOfxStatErrUnknown;
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
                return kOfxStatErrUnknown;
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
                return kOfxStatErrUnknown;
        }
    } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        double *red = va_arg(ap, double *);
        double *green = va_arg(ap, double *);
        double *blue = va_arg(ap, double *);
        double *alpha = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, alpha);

        if (status != kOfxStatOK) {
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, red);
            status = propGetDouble((OfxPropertySetHandle) param_props,
                                   "OfxParamPropDefault",
                                   1,
                                   green);
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 2, blue);
            status = propGetDouble((OfxPropertySetHandle) param_props,
                                   "OfxParamPropDefault",
                                   3,
                                   alpha);
            if (status != kOfxStatOK)
                return kOfxStatErrUnknown;
        }
    } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
        double *red = va_arg(ap, double *);
        double *green = va_arg(ap, double *);
        double *blue = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);

        if (status != kOfxStatOK) {
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, red);
            status = propGetDouble((OfxPropertySetHandle) param_props,
                                   "OfxParamPropDefault",
                                   1,
                                   green);
            status
                = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 2, blue);
            if (status != kOfxStatOK)
                return kOfxStatErrUnknown;
        }
    } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
        double *Y = va_arg(ap, double *);
        double *X = va_arg(ap, double *);

        OfxStatus status
            = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, X);
        status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, Y);

        if (status != kOfxStatOK) {
            status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, X);
            status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 1, Y);
            if (status != kOfxStatOK)
                return kOfxStatErrUnknown;
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

static OfxStatus paramSetValueImpl(OfxParamHandle paramHandle, va_list ap)
{
    mlt_properties param = (mlt_properties) paramHandle;
    char *param_type = mlt_properties_get(param, "t");
    mlt_properties param_props = mlt_properties_get_properties(param, "p");

    if (strcmp(param_type, kOfxParamTypeInteger) == 0
        || strcmp(param_type, kOfxParamTypeChoice) == 0
        || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
        int value = va_arg(ap, int);
        propSetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } else if (strcmp(param_type, kOfxParamTypeDouble) == 0) {
        double value = va_arg(ap, double);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    } else if (strcmp(param_type, kOfxParamTypeRGBA) == 0) {
        mlt_color value = va_arg(ap, mlt_color);

        double red = (double) value.r / 255.0;
        double green = (double) value.g / 255.0;
        double blue = (double) value.b / 255.0;
        double alpha = (double) value.a / 255.0;

        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 3, alpha);
    } else if (strcmp(param_type, kOfxParamTypeRGB) == 0) {
        mlt_color value = va_arg(ap, mlt_color);

        double red = (double) value.r / 255.0;
        double green = (double) value.g / 255.0;
        double blue = (double) value.b / 255.0;

        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, red);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, green);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 2, blue);
    } else if (strcmp(param_type, kOfxParamTypeDouble2D) == 0) {
        mlt_rect value = va_arg(ap, mlt_rect);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value.x);
        propSetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 1, value.y);
    }

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

static OfxStatus multiThread(OfxThreadFunctionV1 func, unsigned int nThreads, void *customArg)
{
    if (!func)
        return kOfxStatFailed;
    func(0, 1, customArg);
    return kOfxStatOK;
}

static OfxStatus multiThreadNumCPUs(unsigned int *nCPUs)
{
    if (!nCPUs)
        return kOfxStatFailed;
    *nCPUs = 1;
    return kOfxStatOK;
}

static OfxStatus multiThreadIndex(unsigned int *threadIndex)
{
    if (!threadIndex)
        return kOfxStatFailed;
    *threadIndex = 0;
    return kOfxStatOK;
}

int multiThreadIsSpawnedThread(void)
{
    return false;
}

static OfxStatus mutexCreate(OfxMutexHandle *mutex, int lockCount)
{
    if (!mutex)
        return kOfxStatFailed;
    // do nothing single threaded
    *mutex = 0;
    return kOfxStatOK;
}

static OfxStatus mutexDestroy(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    // do nothing single threaded
    return kOfxStatOK;
}

static OfxStatus mutexLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    // do nothing single threaded
    return kOfxStatOK;
}

static OfxStatus mutexUnLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    // do nothing single threaded
    return kOfxStatOK;
}

static OfxStatus mutexTryLock(const OfxMutexHandle mutex)
{
    if (!mutex)
        return kOfxStatErrBadHandle;
    // do nothing single threaded
    return kOfxStatOK;
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

static OfxStatus message(
    void *handle, const char *messageType, const char *messageId, const char *format, ...)
{
    return kOfxStatOK;
}

static OfxMessageSuiteV1 MltOfxMessageSuiteV1 = {message};

static OfxStatus setPersistentMessage(
    void *handle, const char *messageType, const char *messageId, const char *format, ...)
{
    return kOfxStatOK;
}

static OfxStatus clearPersistentMessage(void *handle)
{
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

void mltofx_set_source_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format,
                                 double pixel_aspect_ratio,
                                 const char *ofx_depth)
{
    mlt_properties clip;
    mlt_properties clip_prop;

    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Source",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_prop);

    int depth_byte_size = 4;
    char *depth_format = kOfxBitDepthByte;
    char *prop_component = kOfxImageComponentRGBA;
    if (ofx_depth) {
        depth_format = (char *) ofx_depth;
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

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRowBytes, 0, width * depth_byte_size);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPixelDepth, 0, depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedPixelDepth,
                  0,
                  depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropComponents,
                  0,
                  prop_component);
    propSetPointer((OfxPropertySetHandle) clip_prop, kOfxImagePropData, 0, (void *) image);
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

    propSetDouble((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 1, 1.0);

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImageClipPropConnected, 0, 1);
}

void mltofx_set_output_clip_data(OfxPlugin *plugin,
                                 mlt_properties image_effect,
                                 uint8_t *image,
                                 int width,
                                 int height,
                                 mlt_image_format format,
                                 double pixel_aspect_ratio,
                                 const char *ofx_depth)
{
    mlt_properties clip;
    mlt_properties clip_prop;

    clipGetHandle((OfxImageEffectHandle) image_effect,
                  "Output",
                  (OfxImageClipHandle *) &clip,
                  (OfxPropertySetHandle *) &clip_prop);

    int depth_byte_size = 4;
    char *depth_format = kOfxBitDepthByte;
    char *prop_component = kOfxImageComponentRGBA;
    if (ofx_depth) {
        depth_format = (char *) ofx_depth;
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

    propSetInt((OfxPropertySetHandle) clip_prop, kOfxImagePropRowBytes, 0, width * depth_byte_size);
    propSetString((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPixelDepth, 0, depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageClipPropUnmappedPixelDepth,
                  0,
                  depth_format);
    propSetString((OfxPropertySetHandle) clip_prop,
                  kOfxImageEffectPropComponents,
                  0,
                  prop_component);
    propSetPointer((OfxPropertySetHandle) clip_prop, kOfxImagePropData, 0, (void *) image);
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

    propSetDouble((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 1, 1.0);

    propSetDouble((OfxPropertySetHandle) clip_prop,
                  kOfxImagePropPixelAspectRatio,
                  0,
                  pixel_aspect_ratio);
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
        mlt_log_verbose(NULL, "[openfx] Plugin not a filter: %s\n", plugin->pluginIdentifier);
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
    return strcmp(type, kOfxParamTypeInteger2D) && strcmp(type, kOfxParamTypeDouble3D)
           && strcmp(type, kOfxParamTypeInteger3D) && strcmp(type, kOfxParamTypeCustom)
           && strcmp(type, kOfxParamTypeBytes) && strcmp(type, kOfxParamTypePage)
           && strcmp(type, kOfxParamTypePushButton);
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
        snprintf(key, 20, "%d", mlt_param_count++);
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
                mlt_properties_set_int(p, "normalized_coordinates", 0);
            } else if (strcmp(coordinate_system, kOfxParamCoordinatesNormalised) == 0) {
                mlt_properties_set_int(p, "normalized_coordinates", 1);
            }
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
                    || strcmp(param_type, kOfxParamTypeChoice) == 0
                    || strcmp(param_type, kOfxParamTypeBoolean) == 0) {
                    int default_value = 0;
                    propGetInt((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
                    mlt_properties_set_int(p, "default", default_value);
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

                    char default_value[20] = "";
                    sprintf(default_value, "%.4f %.4f", default_value1, default_value2);
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
                    sprintf(default_value,
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
                    sprintf(default_value,
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
                        sprintf(key, "%d", jtr);
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

    mlt_properties_close(clips);
    mlt_properties_close(iparams);
    mlt_properties_close(props);
    return image_effect;
}

void mltofx_param_set_value(mlt_properties params, char *key, mltofx_property_type type, ...)
{
    mlt_properties param = NULL;
    paramGetHandle((OfxParamSetHandle) params, key, (OfxParamHandle *) &param, NULL);

    va_list ap;
    va_start(ap, type);

    switch (type) {
    case mltofx_prop_int: {
        int value = va_arg(ap, int);
        paramSetValue((OfxParamHandle) param, value);
    } break;

    case mltofx_prop_double: {
        double value = va_arg(ap, double);
        paramSetValue((OfxParamHandle) param, value);
    } break;

    case mltofx_prop_string: {
        char *value = va_arg(ap, char *);
        paramSetValue((OfxParamHandle) param, value);
    } break;

    case mltofx_prop_color: {
        mlt_color value = va_arg(ap, mlt_color);
        paramSetValue((OfxParamHandle) param, value);
    } break;

    case mltofx_prop_double2d: {
        mlt_rect value = va_arg(ap, mlt_rect);
        paramSetValue((OfxParamHandle) param, value);
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
    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetClipPreferences,
                                              (OfxImageEffectHandle) image_effect,
                                              NULL,
                                              (OfxPropertySetHandle) get_clippref_args);
    mltofx_log_status_code(status_code, kOfxImageEffectActionGetClipPreferences);
}

void mltofx_get_region_of_definition(OfxPlugin *plugin, mlt_properties image_effect)
{
    mlt_properties get_rod_in_args = mlt_properties_get_properties(image_effect, "get_rod_in_args");

    propSetDouble((OfxPropertySetHandle) get_rod_in_args, kOfxPropTime, 0, 0.0);

    propSetDouble((OfxPropertySetHandle) get_rod_in_args, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) get_rod_in_args, kOfxImageEffectPropRenderScale, 1, 1.0);

    mlt_properties get_rod_out_args = mlt_properties_get_properties(image_effect,
                                                                    "get_rod_out_args");

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetRegionOfDefinition,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) get_rod_in_args,
                                              (OfxPropertySetHandle) get_rod_out_args);

    mltofx_log_status_code(status_code, kOfxImageEffectActionGetRegionOfDefinition);
}

void mltofx_get_regions_of_interest(OfxPlugin *plugin,
                                    mlt_properties image_effect,
                                    double width,
                                    double height)
{
    mlt_properties get_roi_in_args = mlt_properties_get_properties(image_effect, "get_roi_in_args");
    mlt_properties get_roi_out_args = mlt_properties_get_properties(image_effect,
                                                                    "get_roi_out_args");

    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 0, 0.0);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 1, 0.0);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRenderScale, 1, 1.0);

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
                  width);
    propSetDouble((OfxPropertySetHandle) get_roi_in_args,
                  kOfxImageEffectPropRegionOfInterest,
                  3,
                  height);

    OfxStatus status_code = plugin->mainEntry(kOfxImageEffectActionGetRegionsOfInterest,
                                              (OfxImageEffectHandle) image_effect,
                                              (OfxPropertySetHandle) get_roi_in_args,
                                              (OfxPropertySetHandle) get_roi_out_args);
    mltofx_log_status_code(status_code, kOfxImageEffectActionGetRegionsOfInterest);
}

void mltofx_begin_sequence_render(OfxPlugin *plugin, mlt_properties image_effect)
{
    mlt_properties begin_sequence_props = mlt_properties_get_properties(image_effect,
                                                                        "begin_sequence_props");
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  0,
                  0.0);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropFrameRange,
                  1,
                  0.0);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);

    propSetInt((OfxPropertySetHandle) begin_sequence_props, kOfxPropIsInteractive, 0, 0);

    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  0,
                  1.0);
    propSetDouble((OfxPropertySetHandle) begin_sequence_props,
                  kOfxImageEffectPropRenderScale,
                  1,
                  1.0);

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

void mltofx_end_sequence_render(OfxPlugin *plugin, mlt_properties image_effect)
{
    mlt_properties end_sequence_props = mlt_properties_get_properties(image_effect,
                                                                      "end_sequence_props");
    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameRange, 0, 0.0);

    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameRange, 1, 0.0);

    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);

    propSetInt((OfxPropertySetHandle) end_sequence_props, kOfxPropIsInteractive, 0, 0);

    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropRenderScale, 1, 1.0);

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

void mltofx_action_render(OfxPlugin *plugin, mlt_properties image_effect, int width, int height)
{
    mlt_properties render_in_args = mlt_properties_get_properties(image_effect, "render_in_args");
    propSetDouble((OfxPropertySetHandle) render_in_args, kOfxPropTime, 0, 0.0);

    propSetString((OfxPropertySetHandle) render_in_args,
                  kOfxImageEffectPropFieldToRender,
                  0,
                  kOfxImageFieldBoth);

    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 0, 0);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 1, 0);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 2, width);
    propSetInt((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 3, height);

    propSetDouble((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderScale, 0, 1.0);
    propSetDouble((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderScale, 1, 1.0);

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
