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
#include <hb.h>
#include <hb-ft.h>
#include "raqm.h"

int
main (int argc, char* argv[])
{
    const char* fontfile = argv[1];
    const char* text = argv[2];
    raqm_glyph_info_t* info;
    FT_Library ft_library;
    FT_Face face;
    FT_Error ft_error;

    if (argc < 3)
    {
        printf ("Usage: %s font text\n", argv[0]);
        return 1;
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

    info = raqm_shape (text, face, RAQM_DIRECTION_DEFAULT);
    (void) info;

    return 0;
}
