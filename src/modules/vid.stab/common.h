/*
 * common.h
 *
 *  Created on: 20 gru 2013
 *      Author: Jakub Ksiezniak <j.ksiezniak@gmail.com>
 */

#ifndef VIDSTAB_COMMON_H_
#define VIDSTAB_COMMON_H_

extern "C" {
#include <vid.stab/libvidstab.h>
#include <framework/mlt.h>
}

inline VSPixelFormat convertImageFormat(mlt_image_format &format) {
	switch (format) {
	case mlt_image_rgb24:
		return PF_RGB24;
	case mlt_image_rgb24a:
		return PF_RGBA;
	case mlt_image_yuv420p:
		return PF_YUV420P;
	default:
		return PF_NONE;
	}
}

int get_image_and_detect(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable);
int get_image_and_transform(mlt_frame frame, uint8_t **image, mlt_image_format *format, int *width, int *height, int writable);

#endif /* VIDSTAB_COMMON_H_ */
