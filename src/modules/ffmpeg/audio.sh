#!/bin/bash

trap exit

audio_type="$1"
audio_file="$2"
audio_position=$3
audio_frequency=$4
audio_channels=$5
audio_track=$5

if [ "$audio_type" == "dsp" ]
then ffmpeg -ad "$audio_file" -f s16le -ar $audio_frequency -ac $audio_channels -
else ffmpeg -i "$audio_file" -ss $audio_position -f s16le -ar $audio_frequency -ac $audio_channels -
fi

