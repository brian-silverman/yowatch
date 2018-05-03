import re
import sys
from itertools import chain
import os
from glob import glob
from functools import partial
import contextlib

C_FILE = "fonts/fonts.c"
H_FILE = "fonts/_fonts.h"

with contextlib.suppress(FileNotFoundError):
    os.remove(C_FILE)
    os.remove(H_FILE)

#
# For each font file, generate C code
#
font_names = []
for font_index, font_file in enumerate(glob("fonts/*.fnt")):
    font_name = os.path.splitext(os.path.basename(font_file))[0]
    font_codename = chr(ord('a') + font_index)
    font_names.append(font_name)

    #
    # Get a list of pixelated chars from the file.  
    #
    # File is broken into 8 character rows of 16 chars per row.  Each row is
    # separated by a blank line.
    #
    # A row consists of multiple lines euqal to the height of the characters.
    # Characters are separated by " | ".  Each pixel is on if is an "O".
    #
    # Example partial row:
    #
    #  OOO  |   O   | OOOO  | OOOO  | O  O  | OOOOO |  OO
    # O   O |  OO   |     O |     O | O  O  | O     | O  
    # O   O |   O   |  OOO  |  OOOO | OOOOO | OOOO  | OOO   ... for 16 chars
    # O   O |   O   | O     |     O |    O  |     O | O  
    #  OOO  |  OOO  | OOOOO | OOOO  |    O  | OOOO  |  OO
    #
    chars = []
    with open(font_file) as f:
        #
        # Run through entire file, but add a blank line to the end (we key off the
        # blank line to assemble the previous row)
        #
        rows_of_chars = []
        for line in chain(f, ['']):
            if re.search("^\\s*$", line):
                # If blank line, transpose previous row of lines into chars
                chars.extend(zip(*rows_of_chars))
                rows_of_chars = []
            else:
                # Split line by char.  Line has terminating EOL that we must trim
                rows_of_chars.append(re.split(" \\|+ ", line[:-1]))

    #
    # Verify chars look okay
    #
    assert(len(chars) == 128)
    # For each char, width must be the same for each line
    for c in chars:
        assert(len(set(len(line) for line in c)) == 1)
    # Height of each char must be the same
    assert(len(set(len(c) for c in chars)) == 1)
    # Chars must be composed of only space and 'O'
    for i, c in enumerate(chars):
        unique_pixel_chars = sorted(set("".join(c)))
        assert(
            1 <= len(unique_pixel_chars) <= 2 
            and ('O' in unique_pixel_chars or ' ' in unique_pixel_chars)
            )

    #
    # Make a list, for each char, of the first index where that char was used.
    # This will allow us to reduce the font storage whenever there is a duplicate
    # char.
    #
    def char_index(i, c): 
        return (chars[:i].index(c) if c in chars[:i] else i)
    char_refs = [char_index(i, c) for i, c in enumerate(chars)]

    with open(C_FILE, 'a') as f:
        fprint = partial(print, file=f)

        #
        # Header info
        #
        fprint("/*\n * Generated file (by '{:s}') - DO NOT EDIT\n */\n".format(__file__))
        fprint('#include "fonts.h"')

        #
        # Output the C code for the char image once for each unique char.
        #
        for i in sorted(set(char_refs)):
            height = len(chars[i])
            width = len(chars[i][0])
            flatten = (chars[i][h][w] for w in range(width) for h in range(height))
            data = ",".join("0xFFFF" if pixel=='O' else "0x0000" for pixel in flatten)
            fprint("static const uint16 {:s}{:02X}[] = {{{:s}}};".format(font_codename, i, data))

        #
        # Output struct FONT_CHAR array (pointers to the char image, length & width)
        #
        fprint("const struct FONT_CHAR {:s}[MAX_CHARS] = {{".format(font_name))
        fmt = "{{{:s}{:02X},{:d},{:d}}}"
        array = (fmt.format(font_codename, i, len(chars[i][0]), len(chars[i])) for i in char_refs)
        fprint(",\n".join(array))
        fprint("};")

#
# Write master font array
#
with open(C_FILE, 'a') as f:
    fprint = partial(print, file=f)
    fprint("const struct FONT_CHAR *fonts[MAX_FONTS] = {")
    fprint(",\n".join(font_names))
    fprint("};")
    
hfile_template = \
"""\
/*
 * Generated file (by '{:s}') - DO NOT EDIT
 */

#ifndef __FONTS_H_
#define __FONTS_H_

enum {{
{:s}
}};

#endif
"""

with open(H_FILE, 'w') as f:
    enum_string = ",\n".join(fnt.upper() for fnt in font_names + ["MAX_FONTS"])
    f.write(hfile_template.format(__file__, enum_string))
