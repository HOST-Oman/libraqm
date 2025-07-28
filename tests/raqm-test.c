/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
 * Copyright © 2016-2023 Khaled Hosny <khaled@aliftype.com>
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
#ifdef __GNUC__ 
#define  _DEFAULT_SOURCE
#endif
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hb.h>

#include "raqm.h"

static char *text = NULL;
static char *font = NULL;
static char *fonts = NULL;
static char *languages = NULL;
static char *direction = NULL;
static char *features = NULL;
static char *require = NULL;
static char *letterspacing = NULL;
static char *wordspacing = NULL;
static int cluster = -1;
static int position = -1;
static int invisible_glyph = 0;

/* Special exit code, recognized by automake that we're skipping a test. */
static const int skip_exit_status = 77;

typedef struct _font_data
{
  hb_blob_t *blob;
  hb_face_t *face;
  hb_font_t *font;
  void      *data;
  struct _font_data* next;
} font_data_t;

static char*
encode_bytes (const char *bytes)
{
  char *s = (char *) bytes;
  char *p;
  char *ret = (char *) malloc (strlen (bytes) + 1);
  char *r = ret;

  while (s && *s)
  {
    while (*s && strchr (" ", *s))
      s++;
    if (!*s)
      break;

    errno = 0;
    unsigned char b = strtoul (s, &p, 16);
    if (errno || s == p)
    {
      free (ret);
      return NULL;
    }

    *r++ = b;

    s = p;
  }

  *r = '\0';

  return ret;
}

static bool
parse_args (int argc, char **argv)
{
  int i = 1;
  while (i < argc)
  {
    if (strcmp (argv[i], "--text") == 0)
      text = strdup (argv[++i]);
    else if (strcmp (argv[i], "--bytes") == 0)
      text = encode_bytes (argv[++i]);
    else if (strcmp (argv[i], "--font") == 0)
      font = argv[++i];
    else if (strcmp (argv[i], "--fonts") == 0)
      fonts = argv[++i];
    else if (strcmp (argv[i], "--languages") == 0)
      languages = argv[++i];
    else if (strcmp (argv[i], "--direction") == 0)
      direction = argv[++i];
    else if (strcmp (argv[i], "--font-features") == 0)
      features = argv[++i];
    else if (strcmp (argv[i], "--letter-spacing") == 0)
      letterspacing = argv[++i];
    else if (strcmp (argv[i], "--word-spacing") == 0)
      wordspacing = argv[++i];
    else if (strcmp (argv[i], "--require") == 0)
      require = argv[++i];
    else if (strcmp (argv[i], "--cluster") == 0)
      cluster = atoi (argv[++i]);
    else if (strcmp (argv[i], "--position") == 0)
      position = atoi (argv[++i]);
    else if (strcmp (argv[i], "--invisible-glyph") == 0)
      invisible_glyph = atoi (argv[++i]);
    else
    {
      fprintf (stderr, "Unknown option: %s\n", argv[i]);
      return false;
    }
    i++;
  }
  return true;
}

static bool
has_requirement (char *req)
{
  if (strcmp (req, "HB_") > 0)
  {
    long req_ver = strtol (req + strlen ("HB_"), NULL, 10);
    long ver = HB_VERSION_MAJOR*10000 + HB_VERSION_MINOR*100 + HB_VERSION_MICRO;
    return ver >= req_ver;
  }

  fprintf (stderr, "Unknown requirement: %s\n", req);
  return false;
}

static void*
read_file (const char* path, size_t* file_size)
{
    void* mem;
    size_t size, nread;
    FILE* file;

    file = fopen(path, "rb");
    if (!file)
    {
        printf("Failed to load %s\n", path);
        return 0;
    }

    fseek(file, 0, SEEK_END);
    size = (size_t)ftell(file);
    fseek(file, 0, SEEK_SET);

    mem = malloc(size);

    nread = fread(mem, 1, size, file);
    fclose(file);

    if (nread != size)
    {
        fprintf(stderr, "Failed to read %zu bytes from %s\n", size, path);
        free(mem);
        return 0;
    }

    if (file_size)
        *file_size = size;

    return mem;
}

static font_data_t*
load_font(const char* path)
{
    size_t fontdata_size;
    font_data_t *font = (font_data_t*)malloc(sizeof(font_data_t));
    font->next = 0;
    font->data = read_file(path, &fontdata_size);
    assert (font->data != 0);

    font->blob = hb_blob_create(font->data, fontdata_size, HB_MEMORY_MODE_READONLY, 0, 0);
    assert (font->blob != 0);
    font->face = hb_face_create(font->blob, 0);
    assert (font->face != 0);
    font->font = hb_font_create(font->face);
    assert (font->font != 0);

    return font;
}

static void
free_font(font_data_t* font)
{
  if (font->font)
    hb_font_destroy(font->font);
  if (font->face)
    hb_face_destroy(font->face);
  if (font->blob)
    hb_blob_destroy(font->blob);
  free((void*)font->data);
  free((void*)font);
}

int
main (int argc, char **argv)
{
  raqm_t *rq;
  raqm_glyph_t *glyphs;
  size_t count, start_index, index;
  raqm_direction_t dir;
  int x = 0, y = 0;
  font_data_t* fontdata = 0;

  unsigned int major, minor, micro;

  setlocale (LC_ALL, "");

  fprintf (stderr, "Raqm " RAQM_VERSION_STRING "\n");

  raqm_version (&major, &minor, &micro);

  assert (major == RAQM_VERSION_MAJOR);
  assert (minor == RAQM_VERSION_MINOR);
  assert (micro == RAQM_VERSION_MICRO);

  assert (raqm_version_atleast (major, minor, micro));

  if (!parse_args(argc, argv))
    return 1;

  if (text == NULL || (font == NULL && fonts == NULL))
  {
    fprintf (stderr, "Text or font is missing.\n");
    return 1;
  }

  if (require)
  {
    for (char *req = strtok (require, ","); req; req = strtok (NULL, ","))
      if (!has_requirement (req))
        return skip_exit_status;
  }

  dir = RAQM_DIRECTION_DEFAULT;
  if (direction && strcmp(direction, "rtl") == 0)
    dir = RAQM_DIRECTION_RTL;
  else if (direction && strcmp(direction, "ltr") == 0)
    dir = RAQM_DIRECTION_LTR;
  else if (direction && strcmp(direction, "ttb") == 0)
    dir = RAQM_DIRECTION_TTB;

  rq = raqm_create ();
  assert (raqm_set_text_utf8 (rq, text, strlen (text)));
  assert (raqm_set_par_direction (rq, dir));

  if (fonts)
  {
    for (char *tok = strtok (fonts, ","); tok; tok = strtok (NULL, ","))
    {
      int start, length;
      font_data_t* font = load_font(tok);
      assert (font != 0);
      font->next = fontdata;
      start = atoi (strtok (NULL, ","));
      length = atoi (strtok (NULL, ","));
      assert (raqm_set_hb_font_range(rq, font->font, start, length));
    }
  } else {
    fontdata = load_font(font);
    assert (font != 0);
    assert (raqm_set_hb_font (rq, fontdata->font));
  }

  if (languages)
  {
    for (char *tok = strtok (languages, ","); tok; tok = strtok (NULL, ","))
    {
      int start, length;
      start = atoi (strtok (NULL, ","));
      length = atoi (strtok (NULL, ","));
      assert (raqm_set_language(rq, tok, start, length));
    }
  }

  if (features)
  {
    for (char *tok = strtok (features, ","); tok; tok = strtok (NULL, ","))
      assert (raqm_add_font_feature (rq, tok, -1));
  }

  if (letterspacing)
  {
    for (char *tok = strtok (letterspacing, ","); tok; tok = strtok (NULL, ","))
    {
      int spacing = atoi (tok);
      int start, length;
      start = atoi (strtok (NULL, ","));
      length = atoi (strtok (NULL, ","));
      assert (raqm_set_letter_spacing_range (rq, spacing, start, length));
    }
  }

  if (wordspacing)
  {
    for (char *tok = strtok (wordspacing, ","); tok; tok = strtok (NULL, ","))
    {
      int spacing = atoi (tok);
      int start, length;
      start = atoi (strtok (NULL, ","));
      length = atoi (strtok (NULL, ","));
      assert (raqm_set_word_spacing_range (rq, spacing, start, length));
    }
  }

  if (invisible_glyph)
  {
    assert (raqm_set_invisible_glyph (rq, invisible_glyph));
  }

  assert (raqm_layout (rq));

  glyphs = raqm_get_glyphs (rq, &count);
  assert (glyphs != NULL || count == 0);

  if (cluster >= 0)
  {
    index = cluster;
    assert (raqm_index_to_position (rq, &index, &x, &y));
  }

  if (position)
    assert (raqm_position_to_index (rq, position, 0, &start_index));

  free (text);
  raqm_destroy (rq);

  while (fontdata != 0)
  {
    font_data_t* next = fontdata->next;
    free_font (fontdata);
    fontdata = next;
  }

  return 0;
}
