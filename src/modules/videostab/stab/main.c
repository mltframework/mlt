/*
 * Video stabilizer
 *
 * Copyright (c) 2008 Lenny <leonardo.masoni@gmail.com>
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
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
//#include <values.h>

#include "avi/avi.h"

#include "vector.h"
#include "utils.h"
#include "estimate.h"
#include "resample.h"

AviMovie mv_in, mv_out;

void print_help(char *argv[]) {
    
    printf(
        "                                                               \n"
        "Video stabilizer                                               \n"
        "                                                               \n"
        "Usage:                                                         \n"
        " %s [options] <input> <output>                                 \n"
        "                                                               \n"
        "Options:                                                       \n"
        " -r # | Rolling shutter angle | default:   0 | range:  0 - 180 \n"
        " -q # | Output MJPEG quality  | default: 100 | range: 50 - 100 \n"
        "                                                               \n",
        argv[0]
        );

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    int opt_shutter_angle = 0;
    int opt_mjpeg_quality = 100;

    int nf, i, nc, nr;
    int tfs, fps;

    vc *pos_i, *pos_h, *pos_y;

    es_ctx *es;
    rs_ctx *rs;

    opterr = 0;

    while ((i = getopt(argc, argv, "r:q:")) != -1) {

        switch (i) {

            case 'r':
                opt_shutter_angle = atoi(optarg);
                break;

            case 'q':
                opt_mjpeg_quality = atoi(optarg);
                break;

            default:
                print_help(argv);
        }
    }

    if (argc < optind + 2)
        print_help(argv);

    if (AVI_open_movie(argv[optind], &mv_in) != AVI_ERROR_NONE) {

        printf("error: can't read from %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    if (mv_in.header->Streams < 1 || mv_in.streams[0].sh.Type != AVIST_VIDEO) {

        printf("error: video stream not found on %s\n", argv[optind]);
        return EXIT_FAILURE;
    }

    if (AVI_open_compress(argv[optind + 1], &mv_out, 1, AVI_FORMAT_MJPEG) != AVI_ERROR_NONE) {

        printf("error: can't write to %s\n", argv[optind + 1]);
        return EXIT_FAILURE;
    }

    printf("status: setup\n");

    prepare_lanc_kernels();

	nc = mv_in.header->Width;
	nr = mv_in.header->Height;

    tfs = mv_in.header->TotalFrames;
    fps = 1000000 / mv_in.header->MicroSecPerFrame;

    pos_i = (vc *)malloc(tfs * sizeof(vc));
    pos_h = (vc *)malloc(tfs * sizeof(vc));

    pos_y = (vc *)malloc(nr * sizeof(vc));
    
    AVI_set_compress_option(&mv_out, AVI_OPTION_TYPE_MAIN, 0, AVI_OPTION_WIDTH, &nc);
    AVI_set_compress_option(&mv_out, AVI_OPTION_TYPE_MAIN, 0, AVI_OPTION_HEIGHT, &nr);
    AVI_set_compress_option(&mv_out, AVI_OPTION_TYPE_MAIN, 0, AVI_OPTION_FRAMERATE, &fps);
    AVI_set_compress_option(&mv_out, AVI_OPTION_TYPE_MAIN, 0, AVI_OPTION_QUALITY, &opt_mjpeg_quality);

    es = es_init(nc, nr);
    rs = rs_init(nc, nr);

    printf("status: estimating\n");

    for (nf = 0; nf < tfs; nf ++) {

        unsigned char *fr = (unsigned char *)AVI_read_frame(&mv_in, AVI_FORMAT_RGB24, nf, 0);

        pos_i[nf] = vc_add(
            nf > 0 ? pos_i[nf - 1] : vc_set(0.0, 0.0),
            es_estimate(es, fr)
            );
        
        free(fr);

        if ((nf + 1) % 10 == 0) {

            printf(".");
            fflush(stdout);
        }
    }

    printf("\nstatus: filtering\n");

    hipass(pos_i, pos_h, tfs, fps / 2);

    printf("status: resampling\n");

    for (nf = 0; nf < tfs; nf ++) {

        unsigned char *fr = (unsigned char *)AVI_read_frame(&mv_in, AVI_FORMAT_RGB24, nf, 0);

        for (i = 0; i < nr; i ++) {

            pos_y[i] = interp(
                pos_h, tfs,
                nf + (i - nr / 2.0) * opt_shutter_angle / (nr * 360.0)
                );
        }

        rs_resample(rs, fr, pos_y);

        AVI_write_frame(&mv_out, nf, AVI_FORMAT_RGB24, fr, nc * nr * 3 * sizeof(unsigned char));

        if ((nf + 1) % 10 == 0) {

            printf(".");
            fflush(stdout);
        }
    }
        
    printf("\nstatus: closing\n");

    es_free(es);
    rs_free(rs);

    free_lanc_kernels();

    AVI_close(&mv_in);
    AVI_close_compress(&mv_out);

    return EXIT_SUCCESS;
}

