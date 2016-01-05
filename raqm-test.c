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
#include <locale.h>
#include <glib.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "raqm.h"

static gint max_tokens = 50;
static gchar* direction = NULL;
static gchar* text = NULL;
static gchar* font_features = NULL;
static gchar** args = NULL;
static GOptionEntry entries[] =
{
    { "text", 0, 0, G_OPTION_ARG_STRING, &text, "The text to be displayed", "TEXT" },
    { "direction", 0, 0, G_OPTION_ARG_STRING, &direction, "The text direction", "DIR" },
    { "font-features", 0, 0, G_OPTION_ARG_STRING, &font_features, "The font features ", "FEATURES" },
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &args, "Remaining arguments", "FONTFILE" },
    { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
};

int
main (int argc, char* argv[])
{
    unsigned glyph_count;
    raqm_glyph_info_t* info;
    raqm_direction_t raqm_direction;
    FT_Library ft_library;
    FT_Face face;
    FT_Error ft_error;
    gchar** features = NULL;
    GError* error = NULL;
    GOptionContext* context;

    setlocale (LC_ALL, "");
    context = g_option_context_new ("- Test libraqm");
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("Option parsing failed: %s\n", error->message);
        return 1;
    }

    if (text == NULL || args == NULL)
    {
        g_print ("Text or font is missing.\n\n%s", g_option_context_get_help (context, TRUE, NULL));
        return 1;
    }

    /* Initialize FreeType and create FreeType font face. */
    ft_error = FT_Init_FreeType (&ft_library);
    if (ft_error)
    {
        printf ("FT_Init_FreeType() failed.\n");
        return 1;
    }

    ft_error = FT_New_Face (ft_library, args[0], 0, &face);
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

    raqm_direction = RAQM_DIRECTION_DEFAULT;
    if (direction)
    {
        if (strcmp(direction, "rtl") == 0)
        {
            raqm_direction = RAQM_DIRECTION_RTL;
        }
        else if (strcmp(direction, "ltr") == 0)
        {
            raqm_direction = RAQM_DIRECTION_LTR;
        }
    }
    if (font_features)
    {
        features = g_strsplit (font_features, ",", max_tokens);
    }
    glyph_count = raqm_shape (text, (int) strlen (text), face, raqm_direction, (const char **) (features), &info);
    (void) glyph_count;

    g_option_context_free (context);
    raqm_free (info);
    FT_Done_Face (face);
    FT_Done_FreeType (ft_library);

    return 0;
}
