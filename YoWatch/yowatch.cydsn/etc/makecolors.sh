#!/bin/sh
#
# Convert all colors in colors.h to a C readable array of structs, of format:
#
#   {"COLORNAME",COLORNAME},
#
grep "define \w* " colors.h | awk '{print "{\"" $2 "\"," $2 "},"}'
