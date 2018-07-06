#!/bin/sh
echo "#define VERSION \"$(/cmd/git describe --tags --dirty)\"" > _version.h

rm -f etc/colormap.h etc/colors2py etc/colormap.dat
sh etc/makecolors.sh > etc/colormap.h
gcc -I. -Ietc -I.. etc/colors2py.c -o etc/colors2py
etc/colors2py > etc/colormap.dat

python etc/genfonts.py
