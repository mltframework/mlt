/*
 * frei0r_helper.c -- frei0r helper
 * Copyright (c) 2008 Marco Gittler <g.marco@freenet.de>
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
#include "frei0r_helper.h"
#include <frei0r.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

const char *CAIROBLEND_MODE_PROPERTY = "frei0r.cairoblend.mode";

static void rgba_bgra( uint8_t *src, uint8_t* dst, int width, int height )
{
	int n = width * height + 1;

	while ( --n )
	{
		*dst++ = src[2];
		*dst++ = src[1];
		*dst++ = src[0];
		*dst++ = src[3];
		src += 4;
	}	
}

struct update_context {
	f0r_instance_t frei0r;
	int width;
	int height;
	double time;
	uint32_t* inputs[2];
	uint32_t* output;
	void (*f0r_update)  (f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe);
	void (*f0r_update2) (f0r_instance_t instance, double time, const uint32_t* inframe1,
						 const uint32_t* inframe2,const uint32_t* inframe3, uint32_t* outframe);
};

static int f0r_update_slice( int id, int index, int count, void *context )
{
	struct update_context *ctx = context;
	int slice_height = ctx->height / count;
	int slice_bytes = ctx->width * slice_height;
	uint32_t *input = ctx->inputs[0] + index * slice_bytes;
	uint32_t *output = ctx->output + index * slice_bytes;
	ctx->f0r_update(ctx->frei0r, ctx->time, input, output);
	return 0;
}

static int f0r_update2_slice( int id, int index, int count, void *context )
{
	struct update_context *ctx = context;
	int slice_height = ctx->height / count;
	int slice_bytes = ctx->width * slice_height;
	uint32_t *inputs[2] = {
		ctx->inputs[0] + index * slice_bytes,
		ctx->inputs[1] + index * slice_bytes
	};
	uint32_t *output = ctx->output + index * slice_bytes;
	ctx->f0r_update2(ctx->frei0r, ctx->time, inputs[0], inputs[1], NULL, output);
	return 0;
}

int process_frei0r_item( mlt_service service, mlt_position position, double time,
	int length, mlt_frame frame, uint8_t **image, int *width, int *height )
{
	int i=0;
	mlt_properties prop = MLT_SERVICE_PROPERTIES(service);
	f0r_instance_t (*f0r_construct) (unsigned int, unsigned int)
			= mlt_properties_get_data(prop, "f0r_construct", NULL);
	if (!f0r_construct) {
		return -1;
	}
	void (*f0r_update) (f0r_instance_t instance, double time, const uint32_t* inframe, uint32_t* outframe)
			= mlt_properties_get_data(prop, "f0r_update", NULL);
	void (*f0r_get_plugin_info) (f0r_plugin_info_t*)
			= mlt_properties_get_data(prop, "f0r_get_plugin_info", NULL);
	void (*f0r_get_param_info) (f0r_param_info_t* info, int param_index)
			= mlt_properties_get_data(prop, "f0r_get_param_info", NULL);
	void (*f0r_set_param_value) (f0r_instance_t instance, f0r_param_t param, int param_index)
			= mlt_properties_get_data(prop, "f0r_set_param_value", NULL);
	void (*f0r_get_param_value) (f0r_instance_t instance, f0r_param_t param, int param_index)
			= mlt_properties_get_data(prop, "f0r_get_param_value", NULL);
	void (*f0r_update2) (f0r_instance_t instance, double time, const uint32_t* inframe1,
	                     const uint32_t* inframe2, const uint32_t* inframe3, uint32_t* outframe)
			= mlt_properties_get_data(prop, "f0r_update2", NULL);
	mlt_service_type type = mlt_service_identify(service);
	int not_thread_safe = mlt_properties_get_int(prop, "_not_thread_safe");
	int slice_count = mlt_properties_get(prop, "threads") ? mlt_properties_get_int(prop, "threads") : -1;
	const char *service_name = mlt_properties_get(prop, "mlt_service");
	int is_cairoblend = service_name && !strcmp("frei0r.cairoblend", service_name);

	if (slice_count >= 0)
		slice_count = CLAMP(slice_count, 0, mlt_slices_count_normal());

	//use as name the width and height
	int slice_height = *height / (slice_count > 0? slice_count : 1);
	char ctorname[1024] = "";
	if (not_thread_safe)
		sprintf(ctorname, "ctor-%dx%d", *width, slice_height);
	else
#ifdef _WIN32
		sprintf(ctorname, "ctor-%dx%d-%lu", *width, slice_height, GetCurrentThreadId());
#else
		sprintf(ctorname, "ctor-%dx%d-%p", *width, slice_height, (void*) pthread_self());
#endif

	mlt_service_lock(service);

	f0r_instance_t inst = mlt_properties_get_data(prop, ctorname, NULL);
	if (!inst) {
		inst = f0r_construct(*width, slice_height);
		mlt_properties_set_data(prop, ctorname, inst, 0, NULL, NULL);
	}

	if (!not_thread_safe)
		mlt_service_unlock(service);

	f0r_plugin_info_t info;
	memset(&info, 0, sizeof(info));
	if (f0r_get_plugin_info) {
		f0r_get_plugin_info(&info);
		for (i = 0; i < info.num_params; i++) {
			prop = MLT_SERVICE_PROPERTIES(service);
			f0r_param_info_t pinfo;
			f0r_get_param_info(&pinfo,i);
			char index[20];
			snprintf( index, sizeof(index), "%d", i );
			const char *name = index;
			char *val = mlt_properties_get(prop, name);

			// Special cairoblend handling for an override from the cairoblend_mode filter.
			if (is_cairoblend && i == 1) {
				if (mlt_properties_get(MLT_FRAME_PROPERTIES(frame), CAIROBLEND_MODE_PROPERTY)) {
					name = CAIROBLEND_MODE_PROPERTY;
					prop = MLT_FRAME_PROPERTIES(frame);
					val = mlt_properties_get(prop, name);
				} else if (!val && !mlt_properties_get(MLT_FRAME_PROPERTIES(frame), name)) {
					// Reset plugin back to its default value.
					char *default_val = "normal";
					char *plugin_val = NULL;
					f0r_get_param_value(inst, &plugin_val, i);
					if (plugin_val && strcmp(default_val, plugin_val)) {
						f0r_set_param_value(inst, &default_val, i);
						continue;
					}
				}
			}
			if (!val) {
				name = pinfo.name;
				val = mlt_properties_get(prop, name);
			}
			if (!val) {
				// Use the backwards-compatibility param name map.
				mlt_properties map = mlt_properties_get_data(prop, "_param_name_map", NULL);
				if (map) {
					int j;
					for (j = 0; !val && j < mlt_properties_count(map); j++) {
						if (!strcmp(mlt_properties_get_value(map, j), index)) {
							name = mlt_properties_get_name(map, j);
							val = mlt_properties_get(prop, name);
						}
					}
				}
			}
			if (val) {
				switch (pinfo.type) {
					case F0R_PARAM_DOUBLE:
					case F0R_PARAM_BOOL:
					{
						double t = mlt_properties_anim_get_double(prop, name, position, length);
						f0r_set_param_value(inst,&t,i);
						break;
					}
					case F0R_PARAM_COLOR:
					{
						f0r_param_color_t f_color;
						mlt_color m_color = mlt_properties_get(prop, index) ?
							mlt_properties_get_color(prop, index) : mlt_properties_get_color(prop, pinfo.name);
						f_color.r = (float) m_color.r / 255.0f;
						f_color.g = (float) m_color.g / 255.0f;
						f_color.b = (float) m_color.b / 255.0f;
						f0r_set_param_value(inst, &f_color, i);
						break;
					}
					case F0R_PARAM_STRING:
					{
						val = mlt_properties_anim_get(prop, name, position, length);
						f0r_set_param_value(inst, &val, i);
						break;
					}
				}
			}
		}
	}

	int video_area = *width * *height;
	uint32_t *result = mlt_pool_alloc(video_area * sizeof(uint32_t));
	uint32_t *extra = NULL;
	uint32_t *source[2] = { (uint32_t*) image[0], (uint32_t*) image[1] };
	uint32_t *dest = result;

	if (info.color_model == F0R_COLOR_MODEL_BGRA8888) {
		if (type == producer_type) {
			dest = source[0];
		} else {
			rgba_bgra(image[0], (uint8_t*) result, *width, *height);
			source[0] = result;
			dest = (uint32_t*) image[0];
			if (type == transition_type && f0r_update2) {
				extra = mlt_pool_alloc(video_area * sizeof(uint32_t));
				rgba_bgra(image[1], (uint8_t*) extra, *width, *height);
				source[1] = extra;
			}
		}
	}
	if (type == producer_type) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { NULL, NULL },
				.output = dest,
				.f0r_update = f0r_update
			};
			mlt_slices_run_normal(slice_count, f0r_update_slice, &ctx);
		} else {
			f0r_update(inst, time, NULL, dest);
		}
	} else if (type == filter_type) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { source[0], NULL },
				.output = dest,
				.f0r_update = f0r_update
			};
			mlt_slices_run_normal(slice_count, f0r_update_slice, &ctx);
		} else {
			f0r_update(inst, time, source[0], dest);
		}
	} else if (type == transition_type && f0r_update2) {
		if (slice_count > 0) {
			struct update_context ctx = {
				.frei0r = inst,
				.width = *width,
				.height = *height,
				.time = time,
				.inputs = { source[0], source[1] },
				.output = dest,
				.f0r_update2 = f0r_update2
			};
			mlt_slices_run_normal(slice_count, f0r_update2_slice, &ctx);
		} else {
			f0r_update2(inst, time, source[0], source[1], NULL, dest);
		}
	}
	if (not_thread_safe)
		mlt_service_unlock(service);
	if (info.color_model == F0R_COLOR_MODEL_BGRA8888) {
		rgba_bgra((uint8_t*) dest, (uint8_t*) result, *width, *height);
	}
	*image = (uint8_t*) result;
	mlt_frame_set_image(frame, (uint8_t*) result, video_area * sizeof(uint32_t), mlt_pool_release);
	if (extra)
		mlt_pool_release(extra);

	return 0;
}

void destruct (mlt_properties prop ) {

	void (*f0r_destruct) (f0r_instance_t instance) = mlt_properties_get_data(prop, "f0r_destruct", NULL);
	void (*f0r_deinit) (void) = mlt_properties_get_data(prop, "f0r_deinit", NULL);
	int i = 0;

	if (f0r_deinit)
		f0r_deinit();

	for (i=0; i < mlt_properties_count(prop); i++) {
		if (strstr(mlt_properties_get_name(prop, i), "ctor-")) {
			void * inst = mlt_properties_get_data(prop, mlt_properties_get_name(prop, i), NULL);
			if (inst) {
				f0r_destruct((f0r_instance_t) inst);
			}
		}
	}
	void (*dlclose) (void*) = mlt_properties_get_data(prop, "_dlclose", NULL);
	void *handle = mlt_properties_get_data(prop, "_dlclose_handle", NULL);

	if (handle && dlclose)
		dlclose(handle);
}
