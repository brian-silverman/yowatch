#!/bin/bash
ffmpeg -loglevel quiet -y -ar 16k -ac 1 -f s16le -i $1.pcm $1.wav
