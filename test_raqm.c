#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include <hb.h>
#include <hb-ft.h>
#include "raqm.h"

#define FONT_SIZE 36

int main(int argc, char *argv[])
{
    const char *fontfile = argv[1];
    const char *text = argv[2];

    if (argc < 3)
    {
	printf("\n\tError: no/missing input!\n");
	return 1;
    }

    //Initialize FreeType and create FreeType font face.
    FT_Library ft_library;
    FT_Face face;
    FT_Error ft_error;
    if ((ft_error = FT_Init_FreeType (&ft_library)))
	abort();
    if ((ft_error = FT_New_Face (ft_library, fontfile, 0, &face)))
	abort();
    if ((ft_error = FT_Set_Char_Size (face, FONT_SIZE*64, FONT_SIZE*64, 0, 0)))
	abort();


    glyph_info *info = shape_text (text, face);

    printf ("\n\n");
    while (info->index >= 0) {
	printf ("Glyph [%d]: x_advance %d, x_offset %d, y_offset %d\n",
		info->index, info->x_advanced, info->x_offset, info->y_offset );
	info++;
    }


    return 0;
}
