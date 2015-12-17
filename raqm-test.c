/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include "raqm.h"

int
main (int argc, char* argv[])
{
    const char* fontfile;
    const char* direction = "default";
    const char* text;
    raqm_glyph_info_t* info;
    raqm_direction_t raqm_direction;
    FT_Library ft_library;
    FT_Face face;
    FT_Error ft_error;

    if ((argc != 3) && (argc != 4))
    {
        printf ("Usage:\n%s DIRECTION FONT_FILE TEXT\n%s FONT_FILE TEXT\n", argv[0], argv[0]);
        return 1;
    }

    fontfile = (argc == 3) ? argv[1] : argv[2];

    text = (argc == 3) ? argv[2] : argv[3];
    if (strlen(text) == 0)
    {
        printf ("TEXT length is 0.\n");
        return 1;
    }

    if (argc == 4)
    {
        direction = argv[1];
    }

    /* Initialize FreeType and create FreeType font face. */
    ft_error = FT_Init_FreeType (&ft_library);
    if (ft_error)
    {
        printf ("FT_Init_FreeType() failed.\n");
        return 1;
    }

    ft_error = FT_New_Face (ft_library, fontfile, 0, &face);
    if (ft_error)
    {
        printf ("FT_New_Face() failed.\n");
        return 1;
    }

    ft_error = FT_Set_Char_Size (face, face->units_per_EM, face->units_per_EM, 0, 0);
    if (ft_error)
    {
        printf ("FT_Set_Char_Size() failed.\n");
        return 1;
    }

    if (strcmp(direction, "-rtl") == 0 )
    {
        raqm_direction = RAQM_DIRECTION_RTL;
    }
    else if (strcmp(direction, "-ltr") == 0 )
    {
        raqm_direction = RAQM_DIRECTION_LTR;
    }
    else
    {
        raqm_direction = RAQM_DIRECTION_DEFAULT;
    }

    info = raqm_shape (text, face, raqm_direction);
    (void) info;

    return 0;
}
