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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <locale.h>
#include <glib.h>

#include "raqm.h"

static gchar *direction = NULL;
static gchar *text = NULL;
static gchar *features = NULL;
static gchar **args = NULL;
static GOptionEntry entries[] =
{
  { "text", 0, 0, G_OPTION_ARG_STRING, &text, "The text to be displayed", "TEXT" },
  { "direction", 0, 0, G_OPTION_ARG_STRING, &direction, "The text direction", "DIR" },
  { "font-features", 0, 0, G_OPTION_ARG_STRING, &features, "The font features ", "FEATURES" },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &args, "Remaining arguments", "FONTFILE" },
  { NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL }
};

int
main (int argc, char *argv[])
{
  FT_Library library;
  FT_Face face;

  raqm_t *rq;
  raqm_glyph_t *glyphs;
  size_t count;

  GError *error = NULL;
  GOptionContext *context;

  setlocale (LC_ALL, "");
  context = g_option_context_new ("- Test libraqm");
  g_option_context_add_main_entries (context, entries, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_print ("Option parsing failed: %s\n", error->message);
    return 1;
  }

  g_option_context_free (context);

  if (text == NULL || args == NULL)
  {
    g_print ("Text or font is missing.\n\n%s",
             g_option_context_get_help (context, TRUE, NULL));
    return 1;
  }

  if (FT_Init_FreeType (&library))
  {
    printf ("FT_Init_FreeType() failed.\n");
    return 1;
  }

  if (FT_New_Face (library, args[0], 0, &face))
  {
    printf ("FT_New_Face() failed.\n");
    return 1;
  }

  if (FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0))
  {
    printf ("FT_Set_Char_Size() failed.\n");
    return 1;
  }

  rq = raqm_create ();
  raqm_set_text_utf8 (rq, text, strlen (text));
  raqm_set_freetype_face (rq, face, 0, strlen (text));

  if (direction && strcmp(direction, "rtl") == 0)
    raqm_set_par_direction (rq, RAQM_DIRECTION_RTL);
  else if (direction && strcmp(direction, "ltr") == 0)
    raqm_set_par_direction (rq, RAQM_DIRECTION_LTR);
  else
    raqm_set_par_direction (rq, RAQM_DIRECTION_DEFAULT);

  if (features)
  {
    gchar **list = g_strsplit (features, ",", -1);
    for (gchar **p = list; p != NULL && *p != NULL; p++)
      raqm_add_font_feature (rq, *p, -1);
    g_strfreev (list);
  }

  if (!raqm_layout (rq))
  {
    printf ("raqm_layout() failed.\n");
    return 1;
  }

  glyphs = raqm_get_glyphs (rq, &count);
  (void) glyphs;

  raqm_destroy (rq);
  FT_Done_Face (face);
  FT_Done_FreeType (library);

  return 0;
}
