#!/bin/bash
dd conv=swab if=$1.pcm of=$1.flip.pcm
ffmpeg -loglevel quiet -y -ar 16k -ac 1 -f s16le -i $1.flip.pcm $1.wav
