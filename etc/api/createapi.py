#!/usr/local/bin/python3
#
# Create the BLE API command constants in Java format (for the Phone) and C
# header (for the watch), given a python file (api.py) that includes the
# constants
#
from __future__ import print_function
import sys
from functools import partial
from extractedapi import apis

JAVA_MODULE = "BleApi"
C_HEADER_FILE = "../../YoWatch/yowatch.cydsn/bleapi.h"
JAVA_FILE = "../../YoPhone/app/src/main/java/com/readysetstem/yophone/" + JAVA_MODULE + ".java"
MAX_COMMAND_VARNAME="BLE_API_MAX_CMD_NUM"

if (sys.version_info < (3, 0)):
    print("Requires Python 3")
    sys.exit()

# Verify all API are numbers
for a in apis.values():
    assert type(a) == int

# Sort by API number
api_names = sorted(apis, key=lambda x: apis[x])

# Export apis for C
with open(C_HEADER_FILE, "w") as f:
    fprint = partial(print, file=f)
    fprint("/* Generated API numbers */")
    for a in api_names:
        fprint("#define {:<30s} {:d}".format(a, apis[a]))
    fprint("#define {:<30s} {:d}".format(MAX_COMMAND_VARNAME, max(apis.values())))

# Export apis for Java
with open(JAVA_FILE, "w") as f:
    fprint = partial(print, file=f)
    fprint("/* Generated API numbers */")
    fprint("package com.readysetstem.yophone;");
    fprint("public class " + JAVA_MODULE + "{");
    for a in api_names:
        fprint("    public static final int {:<30s} = {:d};".format(a, apis[a]))
    fprint("    public static final int {:<30s} = {:d};"
        .format(MAX_COMMAND_VARNAME, max(apis.values())))
    fprint("}")
