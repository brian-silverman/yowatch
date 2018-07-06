#include <stdio.h>
#include "colors.h"

typedef unsigned short uint16;

struct {
    char * color_name;
    int color_value;
} color_name_map[] = {
#include "colormap.h"
};

int main(int argc, char **argv) {
    printf("{");
    for (int i = 0; i < sizeof(color_name_map)/sizeof(color_name_map[0]); i++) {
        printf("'%s':0x%04X,", color_name_map[i].color_name, color_name_map[i].color_value);
    }
    printf("}\n");
}

