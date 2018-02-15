#!/bin/sh
echo "#define VERSION \"$(/cmd/git describe --tags --dirty)\"" > _version.h

