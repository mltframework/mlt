#!/bin/bash

video_type="$1"
video_file="$2"
video_size="$3"
video_fps=$4
video_position=$5

[ "$video_size" != "" ] && video_size="-s "$video_size

if [ "$video_type" == "v4l" ]
then ffmpeg -vd "$video_file" -r $video_fps $video_size -f imagepipe -f yuv4mpegpipe -
else ffmpeg -i "$video_file" -ss $video_position -f imagepipe -r $video_fps $video_size -f yuv4mpegpipe -
fi
