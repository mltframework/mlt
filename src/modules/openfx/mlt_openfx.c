/*
 * MLT OpenFX
 *
 * Copyright (C) 2024 Meltytech, LLC
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
#include "ofxImageEffect.h"
#include "ofxDrawSuite.h"
#include "ofxProgress.h"
#include "ofxTimeLine.h"
#include "ofxParametricParam.h"
#include <stdarg.h>

static OfxStatus
getPropertySet (OfxImageEffectHandle  imageEffect,
   		OfxPropertySetHandle *propHandle)
{
  mlt_properties image_effect = (mlt_properties) imageEffect;
  *propHandle = mlt_properties_get_data (image_effect, "props", NULL);
   
  return kOfxStatOK;
}

static OfxStatus
getParamSet (OfxImageEffectHandle  imageEffect,
	     OfxParamSetHandle    *paramSet)
{
  if (imageEffect == NULL) return kOfxStatErrBadHandle;

  mlt_properties image_effect = (mlt_properties) imageEffect;
  *paramSet = mlt_properties_get_data (image_effect, "params", NULL);

  return kOfxStatOK;
}

static OfxStatus
clipDefine (OfxImageEffectHandle  imageEffect,
	    const char           *name,	 
	    OfxPropertySetHandle *propertySet)
{
  mlt_log_debug(NULL, "clipDefine: `%s` ## %p\n", name, propertySet);

  mlt_properties image_effect = (mlt_properties) imageEffect;
  mlt_properties set = mlt_properties_get_data (image_effect, "clips", NULL);

  mlt_properties clip = mlt_properties_new ();
  mlt_properties clip_props = mlt_properties_new ();
  if (clip == NULL)
    return kOfxStatErrMemory;

  mlt_properties_set_data(set,
			  name,
			  clip,
			  0,
			  (mlt_destructor) mlt_properties_close,
			  NULL);
  mlt_properties_set_data(clip,
			  "props",
			  clip_props,
			  0,
			  (mlt_destructor) mlt_properties_close,
			  NULL);

  *propertySet = (OfxPropertySetHandle) clip_props;

  return kOfxStatOK;
}

static OfxStatus
clipGetHandle (OfxImageEffectHandle  imageEffect,
	       const char           *name,
	       OfxImageClipHandle   *clip,
	       OfxPropertySetHandle *propertySet)
{
  mlt_properties image_effect = (mlt_properties) imageEffect;
  mlt_properties set = mlt_properties_get_data (image_effect, "clips", NULL);

  mlt_properties clip_temp = mlt_properties_get_data (set, name, NULL);
  *clip = (OfxImageClipHandle) clip_temp;

  if (propertySet != NULL)
    *propertySet = (OfxPropertySetHandle) mlt_properties_get_data (clip_temp, "props", NULL);

  return kOfxStatOK;
}

static OfxStatus
clipGetPropertySet (OfxImageClipHandle    clip,
		    OfxPropertySetHandle *propHandle)
{
  mlt_properties c = (mlt_properties) clip;
  *propHandle = (OfxPropertySetHandle) mlt_properties_get_data (c, "props", NULL);
  
  return kOfxStatOK;
}

static OfxStatus
clipGetImage (OfxImageClipHandle    clip,
	      OfxTime               time,
	      const OfxRectD       *region,
	      OfxPropertySetHandle *imageHandle)
{
  mlt_properties clip_temp = (mlt_properties) clip;
  *imageHandle = (OfxPropertySetHandle) mlt_properties_get_data (clip_temp, "props", NULL);

  /* WIP give him the default region if *region is not NULL */

  return kOfxStatOK;
}

static OfxStatus
clipReleaseImage (OfxPropertySetHandle imageHandle)
{
  return kOfxStatOK;
}

static OfxStatus
clipGetRegionOfDefinition (OfxImageClipHandle  clip,
			   OfxTime             time,
			   OfxRectD           *bounds)
{
  return kOfxStatOK;
}

static int
abortOfxFn (OfxImageEffectHandle imageEffect)
{
  return 0;
}

static OfxStatus
imageMemoryAlloc (OfxImageEffectHandle  instanceHandle, 
		  size_t                nBytes,
		  OfxImageMemoryHandle *memoryHandle)
{
  if (instanceHandle == NULL)
    {
      *memoryHandle = NULL;
      return kOfxStatErrBadHandle;
    }

  void *temp = calloc (1, nBytes);

  if (temp == NULL)
    return kOfxStatErrMemory;

  *memoryHandle = temp;

  return kOfxStatOK;
}

static OfxStatus
imageMemoryFree (OfxImageMemoryHandle memoryHandle)
{
  free (memoryHandle);

  return kOfxStatOK;
}

static OfxStatus
imageMemoryLock (OfxImageMemoryHandle   memoryHandle,
		 void                 **returnedPtr)
{
  return kOfxStatOK;
}

static OfxStatus
imageMemoryUnlock (OfxImageMemoryHandle memoryHandle)
{
  return kOfxStatOK;
}

static OfxImageEffectSuiteV1 MltOfxImageEffectSuiteV1 =
  {
    getPropertySet,
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
    imageMemoryUnlock
  };

#define MLTOFX_DEF_SETTER(fn, fa, vt, ofxproptype) static OfxStatus propSet ## fn (OfxPropertySetHandle  properties, \
								      const char           *property, \
								      int                   index, \
								      fa                    value) \
  {									\
    if (properties == NULL) return kOfxStatErrBadHandle;		\
    if (index < 0) return kOfxStatErrBadIndex;				\
    mlt_properties props = (mlt_properties) properties;			\
    int length = 0;							\
    mlt_properties p = mlt_properties_get_data (props, property, &length); \
    if (p == NULL)							\
      {									\
	mlt_properties pt = mlt_properties_new ();			\
	mlt_properties_set_data (props, property, pt, 0, (mlt_destructor) mlt_properties_close, NULL); \
	mlt_properties_set_int (pt, "t", ofxproptype);			\
	p = pt;								\
      }									\
    vt *v = mlt_properties_get_data (p, "v", &length);			\
    if (length == 0 || length <= index)					\
      {									\
	vt *values = realloc (v, sizeof(vt) * (index+1));		\
	if (values == NULL) return kOfxStatErrMemory;			\
	values[index] = value;						\
	mlt_properties_set_data (p, "v", values, index+1, NULL, NULL);  \
      }									\
    else								\
      {									\
	v[index] = value;						\
      }									\
    return kOfxStatOK;							\
  }

MLTOFX_DEF_SETTER(Pointer, void *, void *, mltofx_prop_pointer);
//MLTOFX_DEF_SETTER(String, const char *, char *, mltofx_prop_string);
MLTOFX_DEF_SETTER(Double, double, double, mltofx_prop_double);
MLTOFX_DEF_SETTER(Int, int, int, mltofx_prop_int);


static OfxStatus propSetString (OfxPropertySetHandle  properties,
				const char           *property,
				int                   index,
				const char           *value)
{
  if (properties == NULL) return kOfxStatErrBadHandle;
  if (index < 0) return kOfxStatErrBadIndex;
  mlt_properties props = (mlt_properties) properties;
  int length = 0;
  mlt_properties p = mlt_properties_get_data (props, property, &length);
  if (p == NULL)
    {
      mlt_properties pt = mlt_properties_new ();
      mlt_properties_set_data (props, property, pt, 0, (mlt_destructor) mlt_properties_close, NULL);
      mlt_properties_set_int (pt, "t", mltofx_prop_string);
      p = pt;
    }
  char **v = mlt_properties_get_data (p, "v", &length);
  if (length == 0 || length <= index)
    {
      char **values = realloc (v, sizeof(char *) * (index+1));
      if (values == NULL) return kOfxStatErrMemory;
      values[index] = strdup(value); /* WIP take care of freeing mem */
      mlt_properties_set_data (p, "v", values, index+1, NULL, NULL);
    }
  else
    {
      v[index] = value;
    }
  return kOfxStatOK;
}

#define MLTOFX_DEF_GETTER(fn, fa) static OfxStatus propGet ## fn (OfxPropertySetHandle  properties, \
								  const char           *property, \
								  int                   index, \
								  fa                   *value) \
  {									\
    if (properties == NULL) return kOfxStatErrBadHandle;		\
    mlt_properties props = (mlt_properties) properties;			\
    int length = 0;							\
    mlt_properties p = mlt_properties_get_data (props, property, &length); \
    if (p != NULL)							\
      {									\
	fa *v = mlt_properties_get_data (p, "v", &length);		\
	if (index < 0 || index >= length) return kOfxStatErrBadIndex;	\
	*value = v[index];						\
	return kOfxStatOK;						\
      }									\
    return kOfxStatErrUnknown;						\
  }

MLTOFX_DEF_GETTER(Pointer, void *);
MLTOFX_DEF_GETTER(String, char *);
MLTOFX_DEF_GETTER(Double, double);
MLTOFX_DEF_GETTER(Int, int);

#define MLTOFX_DEF_SETTER_N(fn, fa, vt, ofxproptype) static OfxStatus propSet ## fn ## N (OfxPropertySetHandle  properties, \
									     const char           *property, \
									     int                   count, \
									     fa                   *value) \
  {									\
    if (properties == NULL) return kOfxStatErrBadHandle;		\
    if (count < 0) return kOfxStatErrBadIndex;				\
    if (value == NULL) return kOfxStatErrValue;				\
    mlt_properties props = (mlt_properties) properties;			\
    int length = 0;							\
    mlt_properties p = mlt_properties_get_data (props, property, &length); \
    if (p == NULL)							\
    {									\
      mlt_properties pt = mlt_properties_new ();			\
      mlt_properties_set_data (props, property, pt, 0, (mlt_destructor) mlt_properties_close, NULL); \
      mlt_properties_set_int (pt, "t", ofxproptype);			\
      p = pt;								\
    }									\
    vt *v = mlt_properties_get_data (p, "v", &length);									\
    vt *values = realloc (v, sizeof(vt) * (count));			\
    if (values == NULL) return kOfxStatErrUnknown;			\
    memcpy (values, value, sizeof(vt) * (count));			\
    mlt_properties_set_data (p, "v", values, count, free, NULL); \
    return kOfxStatOK;							\
  }

MLTOFX_DEF_SETTER_N(Pointer, void *const, void *, mltofx_prop_pointer);
MLTOFX_DEF_SETTER_N(String, const char *const, char *, mltofx_prop_string);
MLTOFX_DEF_SETTER_N(Double, const double, double, mltofx_prop_double);
MLTOFX_DEF_SETTER_N(Int, const int, int, mltofx_prop_int);

#define MLTOFX_DEF_GETTER_N(fn, fa) static OfxStatus propGet ## fn ## N (OfxPropertySetHandle  properties, \
									 const char           *property, \
									 int                   count, \
									 fa                   *value) \
  {									\
    if (properties == NULL) return kOfxStatErrBadHandle;		\
    if (count < 0) return kOfxStatErrBadIndex;				\
    mlt_properties props = (mlt_properties) properties;			\
    int length = 0;							\
    mlt_properties p = mlt_properties_get_data (props, property, &length); \
    if (p == NULL) return kOfxStatErrUnknown;				\
    fa *v = mlt_properties_get_data (p, "v", &length);			\
    if (count > length) return kOfxStatErrUnknown;			\
    memcpy (value, v, sizeof(fa) * (count));				\
    return kOfxStatOK;							\
  }

MLTOFX_DEF_GETTER_N(Pointer, void *);
MLTOFX_DEF_GETTER_N(String, char *);
MLTOFX_DEF_GETTER_N(Double, double);
MLTOFX_DEF_GETTER_N(Int, int);

static OfxStatus
propReset (OfxPropertySetHandle  properties,
	   const char           *property)
{
  if (properties == NULL) return kOfxStatErrBadHandle;

  mlt_properties props = (mlt_properties) properties;
  int length = 0;
  mlt_properties p = mlt_properties_get_data (props, property, &length);
  if (p == NULL) return kOfxStatErrUnknown;

  mltofx_property_type prop_type = 0;
  prop_type = mlt_properties_get_int (p, "t");
  void *values = mlt_properties_get_data (p, "v", &length);

  if (length == 0) return kOfxStatErrUnknown;

  switch (prop_type)
    {
    case mltofx_prop_int:
      memset (values, 0, sizeof (int) * length);
      break;

    case mltofx_prop_double:
      memset (values, 0, sizeof (double) * length);
      break;

    case mltofx_prop_string:
    case mltofx_prop_pointer:
      memset (values, 0, sizeof (void *) * length);
      break;

    default:
      break;
    }

  return kOfxStatOK;
}

static OfxStatus
propGetDimension (OfxPropertySetHandle  properties,
		  const char           *property,
		  int                  *count)
{
  if (properties == NULL) return kOfxStatErrBadHandle;
  mlt_properties props = (mlt_properties) properties;
  mlt_properties p = mlt_properties_get_data (props, property, count);

  if (p == NULL)
    {
      *count = 0;
    }
  else
    {
      int length = 0;
      mlt_properties_get_data (p, "v", &length);
      *count = length;
    }

  return kOfxStatOK;
}

static OfxPropertySuiteV1 MltOfxPropertySuiteV1 =
  {
    propSetPointer,
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
    propGetDimension
  };

static OfxStatus
paramDefine (OfxParamSetHandle     paramSet,
	     const char 	  *paramType,
	     const char		  *name,
	     OfxPropertySetHandle *propertySet)
{
  mlt_log_debug(NULL, "`%s`:`%s`\n", name, paramType);
  if (paramSet == NULL) return kOfxStatErrBadHandle;
  mlt_properties params = (mlt_properties) paramSet;

  int length = 0;
  mlt_properties p = mlt_properties_get_data (params, name, &length);

  if (p != NULL) return kOfxStatErrExists;
  
  if (strcmp(kOfxParamTypeInteger, paramType) == 0   ||
      strcmp(kOfxParamTypeDouble, paramType) == 0    ||
      strcmp(kOfxParamTypeBoolean, paramType) == 0   ||
      strcmp(kOfxParamTypeChoice, paramType) == 0    ||
      strcmp(kOfxParamTypeStrChoice, paramType) == 0 ||
      strcmp(kOfxParamTypeRGBA, paramType) == 0      ||
      strcmp(kOfxParamTypeRGB, paramType) == 0       ||
      strcmp(kOfxParamTypeDouble2D, paramType) == 0  ||
      strcmp(kOfxParamTypeInteger2D, paramType) == 0 ||
      strcmp(kOfxParamTypeDouble3D, paramType) == 0  ||
      strcmp(kOfxParamTypeInteger3D, paramType) == 0 ||
      strcmp(kOfxParamTypeString, paramType) == 0    ||
      strcmp(kOfxParamTypeCustom, paramType) == 0    ||
      strcmp(kOfxParamTypeBytes, paramType) == 0     ||
      strcmp(kOfxParamTypeGroup, paramType) == 0)
    {
      mlt_properties pt = mlt_properties_new ();
      mlt_properties_set_string (pt, "t", paramType);
      mlt_properties param_props = mlt_properties_new ();
      mlt_properties_set_data (pt, "p", param_props, 0, (mlt_destructor) mlt_properties_close, NULL);
      mlt_properties_set_data (params, name, pt, 0, (mlt_destructor) mlt_properties_close, NULL);

      if (propertySet != NULL)
	{
	  *propertySet = (OfxPropertySetHandle) param_props;
	}
    }
  else if (strcmp(kOfxParamTypePage, paramType) == 0 || strcmp(kOfxParamTypePushButton, paramType) == 0)
    {
      return kOfxStatErrUnsupported;
    }
  else
    {
      return kOfxStatErrUnknown;
    }


  return kOfxStatOK;
}

static OfxStatus
paramGetHandle (OfxParamSetHandle     paramSet,
		const char           *name,
		OfxParamHandle       *param,
		OfxPropertySetHandle *propertySet)
{
  if (paramSet == NULL) return kOfxStatErrBadHandle;

  mlt_properties params = (mlt_properties) paramSet;
  int length = 0;
  mlt_properties p = mlt_properties_get_data (params, name, &length);

  if (p == NULL) return kOfxStatErrUnknown;

  *param = (OfxParamHandle) p;

  mlt_properties param_props = mlt_properties_get_data (p, "p", &length);

  if (param_props != NULL && propertySet != NULL)
    {
      *propertySet = (OfxPropertySetHandle) param_props;
    }

  return kOfxStatOK;
}

static OfxStatus
paramSetGetPropertySet (OfxParamSetHandle     paramSet,
			OfxPropertySetHandle *propHandle)
{
  if (paramSet == NULL) return kOfxStatErrBadHandle;

  mlt_properties params = (mlt_properties) paramSet;

  OfxPropertySetHandle plugin_props = mlt_properties_get_data (params, "plugin_props", NULL);

  if (plugin_props == NULL) return kOfxStatErrUnknown;

  *propHandle = plugin_props;

  return kOfxStatOK;
}

static OfxStatus
paramGetPropertySet (OfxParamHandle        param,
		     OfxPropertySetHandle *propHandle)
{
  if (param == NULL) return kOfxStatErrBadHandle;

  mlt_properties p = (mlt_properties) param;
  int length = 0;
  mlt_properties param_props = mlt_properties_get_data (p, "p", &length);

  if (param_props == NULL) return kOfxStatErrUnknown;

  *propHandle = (OfxPropertySetHandle) param_props;

  return kOfxStatOK;
}

static OfxStatus
paramGetValue (OfxParamHandle  paramHandle, ...)
{
  if (paramHandle == NULL) return kOfxStatErrBadHandle;

  va_list ap;
  va_start (ap, paramHandle);

  mlt_properties  param       = (mlt_properties) paramHandle;
  char           *param_type  = mlt_properties_get      (param, "t");
  mlt_properties  param_props = mlt_properties_get_data (param, "p", NULL);

  if (strcmp (param_type, kOfxParamTypeInteger) == 0 ||
      strcmp (param_type, kOfxParamTypeBoolean) == 0 ||
      strcmp (param_type, kOfxParamTypeChoice) == 0)
    {
      int *value = va_arg (ap, int *);
      OfxStatus status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
      if (status != kOfxStatOK)
	{
	  status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeDouble) == 0)
    {
      double *value = va_arg (ap, double *);
      OfxStatus status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);

      if (status != kOfxStatOK)
	{
	  status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeString) == 0 || strcmp (param_type, kOfxParamTypeStrChoice) == 0)
    {
      char **value = va_arg (ap, char **);
      OfxStatus status = propGetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
      if (status != kOfxStatOK)
	{
	  status = propGetString((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeRGBA) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGB) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeCustom) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeBytes) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeGroup) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePage) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePushButton) == 0)
    {
    }

  va_end (ap);
  return kOfxStatOK;
}

static OfxStatus
paramGetValueAtTime (OfxParamHandle  paramHandle,
		     OfxTime         time, ...)
{
  if (paramHandle == NULL) return kOfxStatErrBadHandle;

  va_list ap;
  va_start (ap, time);

  mlt_properties  param       = (mlt_properties) paramHandle;
  char           *param_type  = mlt_properties_get      (param, "t");
  mlt_properties  param_props = mlt_properties_get_data (param, "p", NULL);

  /* WIP make this use mlt_properties_anim_get_* functions maybe */

  if (strcmp (param_type, kOfxParamTypeInteger) == 0 ||
      strcmp (param_type, kOfxParamTypeBoolean) == 0 ||
      strcmp (param_type, kOfxParamTypeChoice) == 0)
    {
      int *value = va_arg (ap, int *);
      OfxStatus status = propGetInt((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
      if (status != kOfxStatOK)
	{
	  status = propGetInt((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeDouble) == 0)
    {
      double *value = va_arg (ap, double *);
      OfxStatus status = propGetDouble((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);

      if (status != kOfxStatOK)
	{
	  status = propGetDouble((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeString) == 0 || strcmp (param_type, kOfxParamTypeStrChoice) == 0)
    {
      char **value = va_arg (ap, char **);
      OfxStatus status = propGetString((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
      if (status != kOfxStatOK)
	{
	  status = propGetString((OfxPropertySetHandle) param_props, "OfxParamPropDefault", 0, value);
	  if (status != kOfxStatOK)
	    {
	      va_end (ap);
	      return kOfxStatErrUnknown;
	    }
	}
    }
  else if (strcmp (param_type, kOfxParamTypeRGBA) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGB) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeCustom) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeBytes) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeGroup) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePage) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePushButton) == 0)
    {
    }

  va_end (ap);
  return kOfxStatOK;
}

static OfxStatus
paramGetDerivative (OfxParamHandle paramHandle,
		    OfxTime        time, ...)
{
  return kOfxStatOK;
}

static OfxStatus
paramGetIntegral (OfxParamHandle paramHandle,
		  OfxTime        time1,
		  OfxTime        time2, ...)
{
  return kOfxStatOK;
}

static OfxStatus
paramSetValue (OfxParamHandle  paramHandle, ...)
{
  if (paramHandle == NULL) return kOfxStatErrBadHandle;

  va_list ap;
  va_start (ap, paramHandle);

  mlt_properties  param       = (mlt_properties) paramHandle;
  char           *param_type  = mlt_properties_get      (param, "t");
  mlt_properties  param_props = mlt_properties_get_data (param, "p", NULL);

  if (strcmp (param_type, kOfxParamTypeInteger) == 0 ||
      strcmp (param_type, kOfxParamTypeChoice)  == 0 ||
      strcmp (param_type, kOfxParamTypeBoolean) == 0)
    {
      int value = va_arg (ap, int);
      propSetInt ((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    }
  else if (strcmp (param_type, kOfxParamTypeDouble) == 0)
    {
      double value = va_arg (ap, double);
      propSetDouble ((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    }
  else if (strcmp (param_type, kOfxParamTypeStrChoice) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGBA) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGB) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeString) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeCustom) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeBytes) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeGroup) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePage) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePushButton) == 0)
    {
    }

  va_end (ap);
  return kOfxStatOK;
}

/* time in frames it seems */
static OfxStatus
paramSetValueAtTime (OfxParamHandle paramHandle,
		     OfxTime        time, ...)
{
  mlt_log_debug(NULL, "<----paramSetValueAtTime---->\n");
  if (paramHandle == NULL) return kOfxStatErrBadHandle;

  /* WIP: animation is not supported yet */

  va_list ap;
  va_start (ap, time);
     
  mlt_properties  param       = (mlt_properties) paramHandle;
  char           *param_type  = mlt_properties_get      (param, "t");
  mlt_properties  param_props = mlt_properties_get_data (param, "p", NULL);
     
  if (strcmp (param_type, kOfxParamTypeInteger) == 0 ||
      strcmp (param_type, kOfxParamTypeChoice)  == 0 ||
      strcmp (param_type, kOfxParamTypeBoolean) == 0)
    {
      int value = va_arg (ap, int);
      propSetInt ((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    }
  else if (strcmp (param_type, kOfxParamTypeDouble) == 0)
    {
      double value = va_arg (ap, double);
      propSetDouble ((OfxPropertySetHandle) param_props, "MltOfxParamValue", 0, value);
    }
  else if (strcmp (param_type, kOfxParamTypeStrChoice) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGBA) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeRGB) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger2D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeDouble3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeInteger3D) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeString) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeCustom) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeBytes) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypeGroup) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePage) == 0)
    {
    }
  else if (strcmp (param_type, kOfxParamTypePushButton) == 0)
    {
    }
     
  va_end (ap);
  return kOfxStatOK;
}

static OfxStatus
paramGetNumKeys (OfxParamHandle  paramHandle,
		 unsigned int   *numberOfKeys)
{
  return kOfxStatOK;
}

static OfxStatus
paramGetKeyTime (OfxParamHandle  paramHandle,
		 unsigned int    nthKey,
		 OfxTime        *time)
{
  return kOfxStatOK;
}

static OfxStatus
paramGetKeyIndex (OfxParamHandle  paramHandle,
		  OfxTime         time,
		  int             direction,
		  int            *index)
{
  return kOfxStatOK;
}

static OfxStatus
paramDeleteKey (OfxParamHandle paramHandle,
		OfxTime        time)
{
  return kOfxStatOK;
}

static OfxStatus
paramDeleteAllKeys (OfxParamHandle  paramHandle)
{
  return kOfxStatOK;
}

static OfxStatus
paramCopy (OfxParamHandle   paramTo,
	   OfxParamHandle   paramFrom,
	   OfxTime          dstOffset,
	   const OfxRangeD *frameRange)
{
  return kOfxStatOK;
}

static OfxStatus
paramEditBegin (OfxParamSetHandle  paramSet,
		const char        *name)
{
  return kOfxStatOK;
}

static OfxStatus
paramEditEnd (OfxParamSetHandle paramSet)
{
  return kOfxStatOK;
}

static OfxParameterSuiteV1 MltOfxParameterSuiteV1 =
  {
    paramDefine,
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
    paramEditEnd
  };

static OfxStatus
memoryAlloc (void    *handle, 
	     size_t   nBytes,
	     void   **allocatedData)
{
  /* handle is ignored */

  void *temp = calloc (1, nBytes);

  if (temp == NULL)
    return kOfxStatErrMemory;

  *allocatedData = temp;

  return kOfxStatOK;
}

static OfxStatus
memoryFree (void *allocatedData)
{
  free (allocatedData);

  return kOfxStatOK;
}

static OfxMemorySuiteV1 MltOfxMemorySuiteV1 = 
  {
    memoryAlloc,
    memoryFree    
  };

static OfxStatus
multiThread (OfxThreadFunctionV1  func,
	     unsigned int         nThreads,
	     void                *customArg)
{
  if (!func) {
    return kOfxStatFailed;
  }

  /* just ignore multi Threading for now use one thread */
  func(0,1,customArg);

  return kOfxStatOK;
}

static OfxStatus
multiThreadNumCPUs (unsigned int *nCPUs)
{
  if (!nCPUs)
    {
      return kOfxStatFailed;
    }

  *nCPUs = 1;

  return kOfxStatOK;
}

static OfxStatus
multiThreadIndex (unsigned int *threadIndex)
{
  if (!threadIndex)
    return kOfxStatFailed;
  *threadIndex = 0;

  return kOfxStatOK;
}

int
multiThreadIsSpawnedThread (void)
{
  return false;
}

static OfxStatus
mutexCreate (OfxMutexHandle *mutex,
	     int             lockCount)
{
  if (!mutex)
    return kOfxStatFailed;

  // do nothing single threaded
  *mutex = 0;

  return kOfxStatOK;
}

static OfxStatus
mutexDestroy (const OfxMutexHandle mutex)
{
  if (mutex != 0)
    return kOfxStatErrBadHandle;
  // do nothing single threaded

  return kOfxStatOK;
}

static OfxStatus
mutexLock (const OfxMutexHandle mutex)
{
  if (mutex != 0)
    return kOfxStatErrBadHandle;
  // do nothing single threaded

  return kOfxStatOK;
}

static OfxStatus
mutexUnLock (const OfxMutexHandle mutex)
{
  if (mutex != 0)
    return kOfxStatErrBadHandle;
  // do nothing single threaded

  return kOfxStatOK;
}

static OfxStatus
mutexTryLock (const OfxMutexHandle mutex)
{
  if (mutex != 0)
    return kOfxStatErrBadHandle;
  // do nothing single threaded

  return kOfxStatOK;
}

static OfxMultiThreadSuiteV1 MltOfxMultiThreadSuiteV1 =
  {
    multiThread,
    multiThreadNumCPUs,
    multiThreadIndex,
    multiThreadIsSpawnedThread,
    mutexCreate,
    mutexDestroy,
    mutexLock,
    mutexUnLock,
    mutexTryLock
  };

static OfxStatus
message (void       *handle,
	 const char *messageType,
	 const char *messageId,
	 const char *format, ...)
{
  return kOfxStatOK;
}

static OfxMessageSuiteV1 MltOfxMessageSuiteV1 =
  {
    message
  };

static OfxStatus
setPersistentMessage (void       *handle,
		      const char *messageType,
		      const char *messageId,
		      const char *format, ...)
{
  return kOfxStatOK;
}

static OfxStatus
clearPersistentMessage (void *handle)
{
  return kOfxStatOK;
}

static OfxMessageSuiteV2 MltOfxMessageSuiteV2 =
  {
    message,			/* Same as the V1 message suite call. */
    setPersistentMessage,
    clearPersistentMessage
  };

static OfxStatus
interactSwapBuffers (OfxInteractHandle interactInstance)
{
  return kOfxStatOK;
}

static OfxStatus
interactRedraw (OfxInteractHandle interactInstance)
{
  return kOfxStatOK;
}

static OfxStatus
interactGetPropertySet (OfxInteractHandle     interactInstance,
			OfxPropertySetHandle *property)
{
  return kOfxStatOK;
}

static OfxInteractSuiteV1 MltOfxInteractSuiteV1 =
  {
    interactSwapBuffers,
    interactRedraw,
    interactGetPropertySet
  };

static OfxStatus
getColour (OfxDrawContextHandle  context,
	   OfxStandardColour     std_colour,
	   OfxRGBAColourF       *colour)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxStatus
setColour (OfxDrawContextHandle  context,
	   const OfxRGBAColourF *colour)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxStatus
setLineWidth (OfxDrawContextHandle context,
	      float                width)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxStatus
setLineStipple (OfxDrawContextHandle      context,
		OfxDrawLineStipplePattern pattern)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxStatus
draw (OfxDrawContextHandle  context,
      OfxDrawPrimitive      primitive,
      const OfxPointD      *points,
      int                   point_count)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxStatus
drawText (OfxDrawContextHandle  context,
	  const char           *text,
	  const OfxPointD      *pos,
	  int                   alignment)
{
  mlt_log_debug(NULL, "OfxDrawSuite `%s`\n", __FUNCTION__);
  return kOfxStatOK;
}

static OfxDrawSuiteV1 MltOfxDrawSuiteV1 =
  {
    getColour,
    setColour,
    setLineWidth,
    setLineStipple,
    draw,
    drawText
  };

static OfxStatus
progressStartV1 (void       *effectInstance,
		 const char *label)
{
  return kOfxStatOK;
}

static OfxStatus
progressUpdateV1 (void   *effectInstance,
		double  progress)
{
  return kOfxStatOK;
}

static OfxStatus
progressEndV1 (void *effectInstance)
{
  return kOfxStatOK;
}

static OfxProgressSuiteV1 MltOfxProgressSuiteV1 =
  {
    progressStartV1,
    progressUpdateV1,
    progressEndV1
  };

static OfxStatus
progressStartV2 (void       *effectInstance,
		 const char *message,
		 const char *messageid)
{
  return kOfxStatOK;
}

static OfxStatus
progressUpdateV2 (void   *effectInstance,
		double  progress)
{
  return kOfxStatOK;
}

static OfxStatus
progressEndV2 (void *effectInstance)
{
  return kOfxStatOK;
}

static OfxProgressSuiteV2 MltOfxProgressSuiteV2 =
  {
    progressStartV2,
    progressUpdateV2,
    progressEndV2
  };

static OfxStatus
getTime (void   *instance,
	 double *time)
{
  return kOfxStatOK;
}

static OfxStatus
gotoTime (void   *instance,
	  double  time)
{
  return kOfxStatOK;
}

static OfxStatus
getTimeBounds (void   *instance,
	       double *firstTime,
	       double *lastTime)
{
  return kOfxStatOK;
}

static OfxTimeLineSuiteV1 MltOfxTimeLineSuiteV1 =
  {
    getTime,
    gotoTime,
    getTimeBounds
  };

static OfxStatus
parametricParamGetValue (OfxParamHandle  param,
			 int             curveIndex,
			 OfxTime         time,
			 double          parametricPosition,
			 double         *returnValue)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamGetNControlPoints(OfxParamHandle  param,
				 int             curveIndex,
				 double          time,
				 int            *returnValue)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamGetNthControlPoint (OfxParamHandle  param,
				   int             curveIndex,
				   double          time,
				   int             nthCtl,
				   double         *key,
				   double         *value)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamSetNthControlPoint (OfxParamHandle param,
				   int            curveIndex,
				   double         time,
				   int            nthCtl,
				   double         key,
				   double         value,
				   bool           addAnimationKey)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamAddControlPoint (OfxParamHandle param,
				int            curveIndex,
				double         time,
				double         key,
				double         value,
				bool           addAnimationKey)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamDeleteControlPoint (OfxParamHandle param,
				   int            curveIndex,
				   int            nthCtl)
{
  return kOfxStatOK;
}

static OfxStatus
parametricParamDeleteAllControlPoints (OfxParamHandle param,
				       int            curveIndex)
{
  return kOfxStatOK;
}

static OfxParametricParameterSuiteV1 MltOfxParametricParameterSuiteV1 =
  {
    parametricParamGetValue,
    parametricParamGetNControlPoints,
    parametricParamGetNthControlPoint,
    parametricParamSetNthControlPoint,
    parametricParamAddControlPoint,
    parametricParamDeleteControlPoint,
    parametricParamDeleteAllControlPoints
  };

const void *
MltOfxfetchSuite (OfxPropertySetHandle  host,
   		  const char           *suiteName,
   		  int                   suiteVersion)
{
  if (strcmp (suiteName, kOfxImageEffectSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxImageEffectSuiteV1;
    }
  else if (strcmp (suiteName, kOfxParameterSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxParameterSuiteV1;
    }
  else if (strcmp (suiteName, kOfxPropertySuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxPropertySuiteV1;
    }
  else if (strcmp (suiteName, kOfxMemorySuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxMemorySuiteV1;
    }
  else if (strcmp (suiteName, kOfxMultiThreadSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxMultiThreadSuiteV1;
    }
  else if (strcmp (suiteName, kOfxMessageSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxMessageSuiteV1;
    }
  else if (strcmp (suiteName, kOfxMessageSuite) == 0 && suiteVersion == 2)
    {
      return &MltOfxMessageSuiteV2;
    }
  else if (strcmp (suiteName, kOfxInteractSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxInteractSuiteV1;
    }
  else if (strcmp (suiteName, kOfxDrawSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxDrawSuiteV1;
    }
  else if (strcmp (suiteName, kOfxProgressSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxProgressSuiteV1;
    }
  else if (strcmp (suiteName, kOfxProgressSuite) == 0 && suiteVersion == 2)
    {
      return &MltOfxProgressSuiteV2;
    }
  else if (strcmp (suiteName, kOfxTimeLineSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxTimeLineSuiteV1;
    }
  else if (strcmp (suiteName, kOfxParametricParameterSuite) == 0 && suiteVersion == 1)
    {
      return &MltOfxParametricParameterSuiteV1;
    }

  mlt_log_debug(NULL, "%s v%d is not supported\n", suiteName, suiteVersion);
  return NULL;
}

void
mltofx_init_host_properties (OfxPropertySetHandle host_properties)
{
  propSetString (host_properties, kOfxPropName, 0, "MLT");
  propSetString (host_properties, kOfxImageEffectPropContext, 0, kOfxImageEffectContextGeneral);
  propSetString (host_properties, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte); /* kOfxBitDepthByte is hardcoded */
  propSetString (host_properties, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
  propSetString (host_properties, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthHalf);
  propSetString (host_properties, kOfxImageEffectPropSupportedPixelDepths, 3, kOfxBitDepthFloat);
}


OfxHost MltOfxHost =
  {
    NULL,
    MltOfxfetchSuite
  };

static void mltofx_log_status_code (OfxStatus code, char *msg)
{
  mlt_log_debug(NULL, "output of `%s` is ", msg);
  switch (code)
    {
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

void
mltofx_action_render (OfxPlugin *plugin, mlt_properties image_effect, int width, int height)
{
  mlt_properties render_in_args  = mlt_properties_get_data (image_effect, "render_in_args",  NULL);
  propSetDouble ((OfxPropertySetHandle) render_in_args, kOfxPropTime, 0, 0.0);

  propSetString ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropFieldToRender, 0, kOfxImageFieldBoth);

  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 0, 0);
  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 1, 0);
  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 2, width);
  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderWindow, 3, height);

  propSetDouble ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderScale, 1, 1.0);

  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropSequentialRenderStatus, 0, 1);

  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropInteractiveRenderStatus, 0, 0);

  propSetInt ((OfxPropertySetHandle) render_in_args, kOfxImageEffectPropRenderQualityDraft, 0, 0);

  OfxStatus status_code =
  plugin->mainEntry (kOfxImageEffectActionRender,
		     (OfxImageEffectHandle) image_effect,
		     (OfxPropertySetHandle) render_in_args,
		     NULL);

  mltofx_log_status_code (status_code, kOfxImageEffectActionRender);
}

void
mltofx_set_output_clip_data (OfxPlugin *plugin, mlt_properties image_effect, uint8_t *image, int width, int height)
{
  mlt_properties clip;
  mlt_properties clip_prop;
  
  clipGetHandle ((OfxImageEffectHandle) image_effect, "Output", (OfxImageClipHandle *) &clip, (OfxPropertySetHandle *) &clip_prop);

  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRowBytes, 0, width*4); /* WIP change this 4 hardcoded depth size */
  propSetString ((OfxPropertySetHandle) clip_prop, "OfxImageEffectPropPixelDepth", 0, kOfxBitDepthByte);
  propSetString ((OfxPropertySetHandle) clip_prop, "OfxImageEffectPropComponents", 0, kOfxImageComponentRGBA);

  propSetPointer ((OfxPropertySetHandle) clip_prop, "OfxImagePropData", 0, (void *) image);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 0, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 1, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 2, width);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 3, height);

  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 0, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 1, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 2, width);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 3, height);

  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPreMultiplication, 0, kOfxImageUnPreMultiplied);
  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImagePropField, 0, kOfxImageFieldNone); /* I'm not sure about this */

  char *tstr = calloc(1, strlen ("Output") + 11);
  sprintf (tstr, "%s%04d%04d", "Output", rand () % 9999, rand () % 9999); /* WIP: do something better */
  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImagePropUniqueIdentifier, 0, tstr);
  free (tstr);

  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 1, 1.0);

  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImagePropPixelAspectRatio, 0, (double) width / (double) height);
}

void
mltofx_set_source_clip_data (OfxPlugin *plugin, mlt_properties image_effect, uint8_t *image, int width, int height)
{
  mlt_properties clip;
  mlt_properties clip_prop;
  
  clipGetHandle ((OfxImageEffectHandle) image_effect, "Source", (OfxImageClipHandle *) &clip, (OfxPropertySetHandle *) &clip_prop);

  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropRowBytes", 0, width*4); /* WIP change this 4 hardcoded depth size */
  propSetString ((OfxPropertySetHandle) clip_prop, "OfxImageEffectPropPixelDepth", 0, kOfxBitDepthByte);
  propSetString ((OfxPropertySetHandle) clip_prop, "OfxImageEffectPropComponents", 0, kOfxImageComponentRGBA);
  propSetPointer ((OfxPropertySetHandle) clip_prop, "OfxImagePropData", 0, (void *) image);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 0, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 1, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 2, width);
  propSetInt ((OfxPropertySetHandle) clip_prop, "OfxImagePropBounds", 3, height);

  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 0, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 1, 0);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 2, width);
  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImagePropRegionOfDefinition, 3, height);

  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImagePropPixelAspectRatio, 0, (double) width / (double) height);

  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropPreMultiplication, 0, kOfxImageUnPreMultiplied);
  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImagePropField, 0, kOfxImageFieldNone); /* I'm not sure about this */

  char *tstr = calloc(1, strlen ("Source") + 11);
  sprintf (tstr, "%s%04d%04d", "Source", rand () % 9999, rand () % 9999); /* WIP: do something better */
  propSetString ((OfxPropertySetHandle) clip_prop, kOfxImagePropUniqueIdentifier, 0, tstr);
  free (tstr);

  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) clip_prop, kOfxImageEffectPropRenderScale, 1, 1.0);

  propSetInt ((OfxPropertySetHandle) clip_prop, kOfxImageClipPropConnected, 0, 1);
}

void
mltofx_get_clip_preferences (OfxPlugin *plugin, mlt_properties image_effect)
{
  mlt_properties get_clippref_args  = mlt_properties_get_data (image_effect, "get_clippref_args",  NULL);
  OfxStatus status_code =
     plugin->mainEntry (kOfxImageEffectActionGetClipPreferences,
     		     (OfxImageEffectHandle) image_effect,
     		     NULL,
     		     (OfxPropertySetHandle) get_clippref_args);
  mltofx_log_status_code (status_code, kOfxImageEffectActionGetClipPreferences);
}

void
mltofx_get_regions_of_interest (OfxPlugin *plugin, mlt_properties image_effect, double width, double height)
{
  mlt_properties get_roi_in_args  = mlt_properties_get_data (image_effect, "get_roi_in_args",  NULL);
  mlt_properties get_roi_out_args = mlt_properties_get_data (image_effect, "get_roi_out_args", NULL);

  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 0, 0.0);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxPropTime, 1, 0.0);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRenderScale, 1, 1.0);

  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRegionOfInterest, 0, 0.0);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRegionOfInterest, 1, 0.0);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRegionOfInterest, 2, width);
  propSetDouble ((OfxPropertySetHandle) get_roi_in_args, kOfxImageEffectPropRegionOfInterest, 3, height);

  OfxStatus status_code =
  plugin->mainEntry (kOfxImageEffectActionGetRegionsOfInterest,
     		     (OfxImageEffectHandle) image_effect,
     		     (OfxPropertySetHandle) get_roi_in_args,
     		     (OfxPropertySetHandle) get_roi_out_args);
  mltofx_log_status_code (status_code, kOfxImageEffectActionGetRegionsOfInterest);
}

void
mltofx_end_sequence_render (OfxPlugin *plugin, mlt_properties image_effect)
{  
  mlt_properties end_sequence_props = mlt_properties_get_data (image_effect, "end_sequence_props", NULL);
  propSetDouble ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameRange, 0, 0.0);
  
  propSetDouble ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameRange, 1, 0.0);
        
  propSetDouble ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);
        
  propSetInt ((OfxPropertySetHandle) end_sequence_props, kOfxPropIsInteractive, 0, 0);
        
  propSetDouble ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropRenderScale, 1, 1.0);
        
  propSetInt ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropSequentialRenderStatus, 0, 1);
        
  propSetInt ((OfxPropertySetHandle) end_sequence_props, kOfxImageEffectPropInteractiveRenderStatus, 0, 0);
        	      
  OfxStatus status_code =
  plugin->mainEntry (kOfxImageEffectActionEndSequenceRender,
		     (OfxImageEffectHandle) image_effect,
		     (OfxPropertySetHandle) end_sequence_props,
		     NULL);
  mltofx_log_status_code (status_code, kOfxImageEffectActionEndSequenceRender);
}

void
mltofx_begin_sequence_render (OfxPlugin *plugin, mlt_properties image_effect)
{  
  mlt_properties begin_sequence_props = mlt_properties_get_data (image_effect, "begin_sequence_props", NULL);
  propSetDouble ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropFrameRange, 0, 0.0);
  propSetDouble ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropFrameRange, 1, 0.0);
  propSetDouble ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropFrameStep, 0, 1.0);

  propSetInt ((OfxPropertySetHandle) begin_sequence_props, kOfxPropIsInteractive, 0, 0);

  propSetDouble ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropRenderScale, 0, 1.0);
  propSetDouble ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropRenderScale, 1, 1.0);

  propSetInt ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropSequentialRenderStatus, 0, 1);

  propSetInt ((OfxPropertySetHandle) begin_sequence_props, kOfxImageEffectPropInteractiveRenderStatus, 0, 0);
  OfxStatus status_code =
    plugin->mainEntry (kOfxImageEffectActionBeginSequenceRender,
		       (OfxImageEffectHandle) image_effect,
		       (OfxPropertySetHandle) begin_sequence_props,
		       NULL);
  mltofx_log_status_code (status_code, kOfxImageEffectActionBeginSequenceRender);
}

void
mltofx_create_instance (OfxPlugin *plugin, mlt_properties image_effect)
{
  OfxStatus status_code =
  plugin->mainEntry (kOfxActionCreateInstance, (OfxImageEffectHandle) image_effect, NULL, NULL);
  mltofx_log_status_code (status_code, kOfxActionCreateInstance);
}

void
mltofx_destroy_instance (OfxPlugin *plugin, mlt_properties image_effect)
{
  OfxStatus status_code =
    plugin->mainEntry (kOfxActionDestroyInstance, (OfxImageEffectHandle) image_effect, NULL, NULL);
  mltofx_log_status_code (status_code, kOfxActionDestroyInstance);
}

void
mltofx_param_set_value (mlt_properties params, char *key, mltofx_property_type type, ...)
{
  mlt_properties param = NULL;
  paramGetHandle ((OfxParamSetHandle) params, key, (OfxParamHandle *) &param, NULL);

  /* WIP: maybe it will be better idea to use paramSetValueAtTime */

  va_list ap;
  va_start (ap, type);

  switch (type)
    {
    case mltofx_prop_int:
      {
	int value = va_arg (ap, int);
	paramSetValue ((OfxParamHandle) param, value);
      }
      break;

    case mltofx_prop_double:
      {
	double value = va_arg (ap, double);
	paramSetValue ((OfxParamHandle) param, value);
      }
      break;

    case mltofx_prop_string:
      {
	char *value = va_arg (ap, char *);
	paramSetValue ((OfxParamHandle) param, value);
      }
      break;

    default:
      break;
    }

  va_end (ap);
}

void *
mltofx_fetch_params (OfxPlugin      *plugin,
		     mlt_properties  params)
{
  mlt_properties image_effect = mlt_properties_new ();
  mlt_properties clips  = mlt_properties_new ();
  mlt_properties props  = mlt_properties_new ();
  mlt_properties iparams = mlt_properties_new ();

  mlt_properties_set_data (image_effect, "clips", clips, 0,    (mlt_destructor) mlt_properties_close, NULL);
  mlt_properties_set_data (image_effect, "props", props, 0,    (mlt_destructor) mlt_properties_close, NULL);
  mlt_properties_set_data (image_effect, "params", iparams, 0, (mlt_destructor) mlt_properties_close, NULL);
  mlt_properties_set_data (iparams, "plugin_props", props, 0,  (mlt_destructor) mlt_properties_close, NULL);
  mlt_properties_set_data (image_effect, "mltofx_params", params, 0,  (mlt_destructor) mlt_properties_close, NULL);

  propSetString ((OfxPropertySetHandle) props, kOfxImageEffectPropContext, 0, kOfxImageEffectContextGeneral);

  plugin->setHost (&MltOfxHost);

  OfxStatus status_code = kOfxStatErrUnknown;
  status_code = plugin->mainEntry (kOfxActionLoad, NULL, NULL, NULL);
  mltofx_log_status_code (status_code, "kOfxActionLoad");

  status_code = plugin->mainEntry (kOfxActionDescribe, (OfxImageEffectHandle) image_effect, NULL, NULL);
  mltofx_log_status_code (status_code, "kOfxActionDescribe");

  status_code = plugin->mainEntry (kOfxImageEffectActionDescribeInContext, (OfxImageEffectHandle) image_effect, (OfxPropertySetHandle) MltOfxHost.host, NULL);
  mltofx_log_status_code (status_code, "kOfxImageEffectActionDescribeInContext");

  int iparams_length = mlt_properties_count (iparams);
  int ipiter;

  /* starting with 1 to skip the plugin_props */
  for (ipiter = 1; ipiter < iparams_length; ++ipiter)
    {
      char *name = mlt_properties_get_name(iparams, ipiter);
      mlt_properties pp = mlt_properties_get_data (iparams, name, NULL);
      char *pt = mlt_properties_get (pp, "t");

      mlt_properties ppp = mlt_properties_get_data (pp, "p", NULL);

      int is_secret = -1;
      propGetInt ((OfxPropertySetHandle) ppp, kOfxParamPropSecret, 0, &is_secret);
      if (is_secret == 1) continue;

      mlt_properties p = mlt_properties_new ();

      mlt_properties_set_data(params,
			      name,
			      p,
			      0,
			      (mlt_destructor) mlt_properties_close,
			      NULL);

      int p_length = mlt_properties_count (ppp);
      
      if (strcmp (pt, kOfxParamTypeInteger) == 0)
	{
	  mlt_properties_set (p, "type", "integer");
	}
      else if (strcmp (pt, kOfxParamTypeDouble) == 0)
	{
	  mlt_properties_set (p, "type", "double");
	}
      else if (strcmp (pt, kOfxParamTypeChoice) == 0)
	{
	  mlt_properties_set (p, "type", "integer");
	}
      else if (strcmp (pt, kOfxParamTypeString) == 0)
      /* else if (strcmp (pt, kOfxParamTypeChoice) == 0 ||
         	       strcmp (pt, kOfxParamTypeString) == 0) */
	{
	  mlt_properties_set (p, "type", "string");
	}
      else if (strcmp (pt, kOfxParamTypeBoolean) == 0)
	{
	  mlt_properties_set (p, "type", "boolean");
	}
      else if (strcmp (pt, kOfxParamTypeGroup) == 0)
	{
	  mlt_properties_set (p, "type", "group");
	  int opened = 0;
	  propGetInt ((OfxPropertySetHandle) ppp, kOfxParamPropGroupOpen, 0, &opened);
	  mlt_properties_set_int (p, "opened", opened);
	}

      int animation = 1;
      propGetInt ((OfxPropertySetHandle) ppp, kOfxParamPropAnimates, 0, &animation);
      mlt_properties_set (p, "animation", animation ? "yes" : "no");

      int jt;
      for (jt = 0; jt < p_length; ++jt)
	{
	  char *p_name = mlt_properties_get_name(ppp, jt);

	  if (strcmp (p_name, kOfxParamPropDefault) == 0)
	    {
	      if (strcmp (pt, kOfxParamTypeInteger) == 0 ||
		  strcmp (pt, kOfxParamTypeChoice)  == 0 ||
		  strcmp (pt, kOfxParamTypeBoolean) == 0)
		{
		  int default_value = 0;
		  propGetInt ((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
		  mlt_properties_set_int (p, "default", default_value);
		}
	      else if (strcmp (pt, kOfxParamTypeDouble) == 0)
		{
		  double default_value = 0.0;
		  propGetDouble ((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
		  mlt_properties_set_double (p, "default", default_value);
		}
	      else if (strcmp (pt, kOfxParamTypeString) == 0)
		{
		  char *default_value = "";
		  propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &default_value);
		  mlt_properties_set (p, "default", default_value);
		}

	      if (strcmp (pt, kOfxParamTypeChoice) == 0)
		{
		  char key[20];
		  int count = 0;
		  mlt_properties choices = mlt_properties_new ();
		  mlt_properties_set_data (p, "values", choices, 0, (mlt_destructor) mlt_properties_close, NULL);

		  propGetDimension ((OfxPropertySetHandle) ppp, kOfxParamPropChoiceOption, &count);

		  int jtr;
		  for (jtr = 0; jtr < count; ++jtr)
		    {
		      key[0] = '\0';
		      sprintf (key, "%d", jtr);
		      char *choice_str = NULL;
		      propGetString ((OfxPropertySetHandle) ppp, kOfxParamPropChoiceOption, jtr, &choice_str);
		      mlt_properties_set (choices, key, choice_str);
		    }
		}
	    }
	  else if (strcmp (p_name, kOfxParamPropMin) == 0)
	    {
	      if (strcmp (pt, kOfxParamTypeInteger) == 0 ||
		  strcmp (pt, kOfxParamTypeChoice)  == 0 ||
		  strcmp (pt, kOfxParamTypeBoolean) == 0)
		{
		  int minimum_value = 0;
		  propGetInt ((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
		  mlt_properties_set_int (p, "minimum", minimum_value);
		}
	      else if (strcmp (pt, kOfxParamTypeDouble) == 0)
		{
		  double minimum_value = 0.0;
		  propGetDouble ((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
		  mlt_properties_set_double (p, "minimum", minimum_value);
		}
	      else if (strcmp (pt, kOfxParamTypeString) == 0)
		{
		  char *minimum_value = "";
		  propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &minimum_value);
		  mlt_properties_set (p, "minimum", minimum_value);
		}
	    }
	  else if (strcmp (p_name, kOfxParamPropMax) == 0)
	    {
	      if (strcmp (pt, kOfxParamTypeInteger) == 0 ||
		  strcmp (pt, kOfxParamTypeChoice)  == 0 ||
		  strcmp (pt, kOfxParamTypeBoolean) == 0)
		{
		  int maximum_value = 0;
		  propGetInt ((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
		  mlt_properties_set_int (p, "maximum", maximum_value);
		}
	      else if (strcmp (pt, kOfxParamTypeDouble) == 0)
		{
		  double maximum_value = 0.0;
		  propGetDouble ((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
		  mlt_properties_set_double (p, "maximum", maximum_value);
		}
	      else if (strcmp (pt, kOfxParamTypeString) == 0)
		{
		  char *maximum_value = "";
		  propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &maximum_value);
		  mlt_properties_set (p, "maximum", maximum_value);
		}
	    }
	  else if (strcmp (p_name, kOfxParamPropStringMode) == 0)
	    {
	      char *str_value = "";
	      propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &str_value);
	      if (strcmp (str_value, kOfxParamStringIsLabel) == 0)
		{
		  mlt_properties_set (p, "readonly", "yes");
		}
	    }
	  else if (strcmp (p_name, kOfxParamPropParent) == 0)
	    {
	      char *str_value = "";
	      propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &str_value);
	      mlt_properties_set (p, "group", str_value);
	    }
	  else if (strcmp (p_name, kOfxPropLabel) == 0)
	    {
	      char *str_value = "";
	      propGetString ((OfxPropertySetHandle) ppp, p_name, 0, &str_value);
	      mlt_properties_set (p, "title", str_value);
	    }
	}
    }

  return image_effect;
}
