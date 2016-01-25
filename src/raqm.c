/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include <hb.h>
#include <hb-ft.h>

#include "raqm.h"
#include "reorder_runs.h"

/**
 * SECTION:raqm
 * @title: Raqm
 * @short_description: A library for complex text layout
 * @include: raqm.h
 *
 * Raqm is a light weight text layout library with strong emphasis on
 * supporting languages and writing systems that require complex text layout.
 *
 * The main object in Raqm API is #raqm_t, it stores all the states of the
 * input text, its properties, and the output of the layout process.
 *
 * To start, you create a #raqm_t object, add text and font(s) to it, run the
 * layout process, and finally query about the output. For example:
 *
 * |[<!-- language="C" -->
 * unit32_t     *text;
 * size_t        len;
 * FT_Face       face;
 *
 * raqm_t       *rq;
 * raqm_glyph_t *glyphs;
 * size_t        nglyph;
 *
 * text = … ;
 * len = … ;
 * face = … ;
 *
 * rq = raqm_create ();
 * raqm_set_text (rq, text, len);
 * raqm_set_freetype_face (rq, face);
 * if (raqm_layout (rq))
 * {
 *   glyphs = raqm_get_glyphs (rq, &nglyphs);
 *   …
 * }
 * ]|
 */

/* For enabling debug mode */
/*#define RAQM_DEBUG 1*/
#ifdef RAQM_DEBUG
#define RAQM_DBG(...) fprintf (stderr, __VA_ARGS__)
#else
#define RAQM_DBG(...)
#endif

#ifdef RAQM_TESTING
# define RAQM_TEST(...) printf (__VA_ARGS__)
# define SCRIPT_TO_STRING(script) \
    char buff[5]; \
    hb_tag_to_string (hb_script_to_iso15924_tag (script), buff); \
    buff[4] = '\0';
#else
# define RAQM_TEST(...)
#endif

/* FIXME: fix multi-font support */
/* #define RAQM_MULTI_FONT */

typedef enum {
  RAQM_FLAG_NONE = 0,
  RAQM_FLAG_UTF8 = 1 << 0
} raqm_flags_t;

typedef struct _raqm_run raqm_run_t;

struct _raqm {
  int              ref_count;

  uint32_t        *text;
  size_t           text_len;

  raqm_direction_t base_dir;

  hb_feature_t    *features;
  size_t           features_len;

  hb_script_t     *scripts;
#ifdef RAQM_MULTI_FONT
  hb_font_t      **fonts;
#else
  hb_font_t       *font;
#endif

  raqm_run_t      *runs;
  raqm_glyph_t    *glyphs;

  raqm_flags_t     flags;
};

struct _raqm_run {
  int            pos;
  int            len;

  hb_direction_t direction;
  hb_script_t    script;
  hb_font_t     *font;
  hb_buffer_t   *buffer;

  raqm_run_t    *next;
};

/**
 * raqm_create:
 *
 * Creates a new #raqm_t with all its internal states initialized to their
 * defaults.
 *
 * Return value:
 * A newly allocated #raqm_t with a reference count of 1. The initial reference
 * count should be released with raqm_destroy() when you are done using the
 * #raqm_t. Returns %NULL in case of error.
 *
 * Since: 0.1
 */
raqm_t *
raqm_create (void)
{
  raqm_t *rq;

  rq = malloc (sizeof (raqm_t));
  if (!rq)
    return NULL;

  rq->ref_count = 1;

  rq->text = NULL;
  rq->text_len = 0;

  rq->base_dir = RAQM_DIRECTION_DEFAULT;

  rq->features = NULL;
  rq->features_len = 0;

  rq->scripts = NULL;

#ifdef RAQM_MULTI_FONT
  rq->fonts = NULL;
#else
  rq->font = NULL;
#endif

  rq->runs = NULL;
  rq->glyphs = NULL;

  rq->flags = RAQM_FLAG_NONE;

  return rq;
}

/**
 * raqm_reference:
 * @rq: a #raqm_t.
 *
 * Increases the reference count on @rq by one. This prevents @rq from being
 * destroyed until a matching call to raqm_destroy() is made.
 *
 * Return value:
 * The referenced #raqm_t.
 *
 * Since: 0.1
 */
raqm_t *
raqm_reference (raqm_t *rq)
{
  if (rq)
    rq->ref_count++;

  return rq;
}

static void
_raqm_free_runs (raqm_t *rq)
{
  raqm_run_t *runs = rq->runs;
  while (runs)
  {
    raqm_run_t *run = runs;
    runs = runs->next;

    hb_buffer_destroy (run->buffer);
    free (run);
  }
}

#ifdef RAQM_MULTI_FONT
static void
_raqm_free_fonts (raqm_t *rq)
{
  if (!rq->fonts)
    return;

  for (size_t i = 0; i < rq->text_len; i++)
  {
    if (rq->fonts[i])
      hb_font_destroy (rq->fonts[i]);
  }
}
#endif

/**
 * raqm_destroy:
 * @rq: a #raqm_t.
 *
 * Decreases the reference count on @rq by one. If the result is zero, then @rq
 * and all associated resources are freed.
 * See cairo_reference().
 *
 * Since: 0.1
 */
void
raqm_destroy (raqm_t *rq)
{
  if (!rq || --rq->ref_count != 0)
    return;

  free (rq->text);
  free (rq->scripts);
#ifdef RAQM_MULTI_FONT
  _raqm_free_fonts (rq);
  free (rq->fonts);
#else
  hb_font_destroy (rq->font);
#endif
  _raqm_free_runs (rq);
  free (rq->glyphs);
  free (rq);
}

/**
 * raqm_set_text:
 * @rq: a #raqm_t.
 * @text: a UTF-32 encoded text string.
 * @len: the length of @text.
 *
 * Adds @text to @rq to be used for layout. It must be a valid UTF-32 text, any
 * invalid character will be replaced with U+FFFD. The text should typically
 * represent a full paragraph, since doing the layout of chunks of text
 * separately can give improper output.
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_set_text (raqm_t         *rq,
               const uint32_t *text,
               size_t          len)
{
  if (!rq || !text || !len)
    return false;

  free (rq->text);

  rq->text_len = len;
  rq->text = malloc (sizeof (uint32_t) * rq->text_len);
  if (!rq->text)
    return NULL;
  memcpy (rq->text, text, sizeof (uint32_t) * rq->text_len);

  return true;
}

/**
 * raqm_set_text_utf8:
 * @rq: a #raqm_t.
 * @text: a UTF-8 encoded text string.
 * @len: the length of @text.
 *
 * Same as raqm_set_text(), but for text encoded in UTF-8 encoding.
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_set_text_utf8 (raqm_t         *rq,
                    const char     *text,
                    size_t          len)
{
  uint32_t unicode[len];
  size_t ulen;

  if (!rq || !text || !len)
    return false;

  RAQM_TEST ("Text is: %s\n", text);

  rq->flags |= RAQM_FLAG_UTF8;

  ulen = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8,
                                     text, len, unicode);

  return raqm_set_text (rq, unicode, ulen);
}

/**
 * raqm_set_par_direction:
 * @rq: a #raqm_t.
 * @dir: the direction of the paragraph.
 *
 * Sets the paragraph direction, also known as block direction in CSS. For
 * horizontal text, this controls the overall direction in the Unicode
 * Bidirectional Algorithm, so when the text is mainly right-to-left (with or
 * without some left-to-right) text, then the base direction should be set to
 * #RAQM_DIRECTION_RTL and vice versa.
 *
 * The default is #RAQM_DIRECTION_DEFAULT, which determines the paragraph
 * direction based on the first character with strong bidi type (see [rule
 * P2](http://unicode.org/reports/tr9/#P2) in Unicode Bidirectional Algorithm),
 * which can be good enough for many cases but has problems when a mainly
 * right-to-left paragraph starts with a left-to-right character and vice versa
 * as the detected paragraph direction will be the wrong one, or when text does
 * not contain any characters with string bidi types (e.g. only punctuation or
 * numbers) as this will default to left-to-right paragraph direction.
 *
 * For vertical, top-to-bottom text, #RAQM_DIRECTION_TTB should be used. Raqm,
 * however, provides limited vertical text support and does not handle rotated
 * horizontal text in vertical text, instead everything is treated as vertical
 * text.
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_set_par_direction (raqm_t          *rq,
                        raqm_direction_t dir)
{
  if (!rq)
    return false;

  rq->base_dir = dir;

  return true;
}

/**
 * raqm_add_font_feature:
 * @rq: a #raqm_t.
 * @feature: (transfer none): a font feature string.
 * @len: length of @feature, -1 for %NULL-terminated.
 *
 * Adds a font feature to be used by the #raqm_t during text layout. This is
 * usually used to turn on optional font features that are not enabled by
 * default, for example `dlig` or `ss01`, but can be also used to turn off
 * default font features.
 *
 * @feature is string representing a single font feature, in the syntax
 * understood by hb_feature_from_string().
 *
 * This function can be called repeatedly, new features will be appended to the
 * end of the features list and can potentially override previous features.
 *
 * Return value:
 * %true if parsing @feature succeeded, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_add_font_feature (raqm_t     *rq,
                       const char *feature,
                       int         len)
{
  hb_bool_t ok;
  hb_feature_t fea;

  if (!rq)
    return false;

  ok = hb_feature_from_string (feature, len, &fea);
  if (ok)
  {
    rq->features_len++;
    rq->features = realloc (rq->features,
                            sizeof (hb_feature_t) * (rq->features_len));
    if (!rq->features)
      return false;

    rq->features[rq->features_len - 1] = fea;
  }

  return ok;
}

#ifdef HAVE_HB_FT_FONT_CREATE_REFERENCED
# define HB_FT_FONT_CREATE(a) hb_ft_font_create_referenced (a)
#else
static hb_font_t *
_raqm_hb_ft_font_create_referenced (FT_Face face)
{
  FT_Reference_Face (face);
  return hb_ft_font_create (face, (hb_destroy_func_t) FT_Done_Face);
}
# define HB_FT_FONT_CREATE(a) _raqm_hb_ft_font_create_referenced (a)
#endif

/**
 * raqm_set_freetype_face:
 * @rq: a #raqm_t.
 * @face: an #FT_Face.
 *
 * Sets an #FT_Face to be used for all characters in @rq.
 *
 * See also raqm_set_freetype_face_range().
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_set_freetype_face (raqm_t *rq,
                        FT_Face face)
{
  return raqm_set_freetype_face_range (rq, face, 0, rq->text_len);
}

/**
 * raqm_set_freetype_face_range:
 * @rq: a #raqm_t.
 * @face: an #FT_Face.
 * @start: index of first character that should use @face.
 * @len: number of characters using @face.
 *
 * Sets an #FT_Face to be used for @len-number of characters staring at @start.
 * The @start and @len are UTF-32 character indices, regardless of the original
 * text encoding.
 *
 * This method can be used repeatedly to set different faces for different
 * parts of the text. It is the responsibility of the client to make sure that
 * face ranges cover the whole text.
 *
 * See also raqm_set_freetype_face().
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Stability:
 * Unstable
 *
 * Since: 0.1
 */
bool
raqm_set_freetype_face_range (raqm_t *rq,
                              FT_Face face,
                              size_t  start,
                              size_t  len)
{
  if (!rq || !rq->text_len || start >= rq->text_len)
    return false;

  if (start + len > rq->text_len)
    return false;

#ifdef RAQM_MULTI_FONT
  if (!rq->fonts)
    rq->fonts = calloc (sizeof (intptr_t), rq->text_len);
  if (!rq->fonts)
    return NULL;

  for (size_t i = 0; i < len; i++)
  {
    if (rq->fonts[start + i])
      hb_font_destroy (rq->fonts[start + i]);
    rq->fonts[start + i] = HB_FT_FONT_CREATE (face);
  }
#else
  if (rq->font)
    hb_font_destroy (rq->font);
  rq->font = HB_FT_FONT_CREATE (face);
#endif

  return true;
}

static bool
_raqm_itemize (raqm_t *rq);

static bool
_raqm_shape (raqm_t *rq);

/**
 * raqm_layout:
 * @rq: a #raqm_t.
 *
 * Run the text layout process on @rq. This is the main Raqm function where the
 * Unicode Bidirectional Text algorithm will be applied to the text in @rq,
 * text shaping, and any other part of the layout process.
 *
 * Return value:
 * %true if the layout process was successful, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_layout (raqm_t *rq)
{
  if (!rq || !rq->text_len || !rq->font)
    return false;

  if (!_raqm_itemize (rq))
    return false;

  if (!_raqm_shape (rq))
    return false;

  return true;
}

static uint32_t
_raqm_u32_to_u8_index (raqm_t   *rq,
                       uint32_t  index);

/**
 * raqm_get_glyphs:
 * @rq: a #raqm_t.
 * @length: (out): output array length.
 *
 * Gets the final result of Raqm layout process, an array of #raqm_glyph_t
 * containing the glyph indices in the font, their positions and other possible
 * information.
 *
 * Return value: (transfer none):
 * An array of #raqm_glyph_t, or %NULL in case of error. This is owned by @rq
 * and must not be freed.
 *
 * Since: 0.1
 */
raqm_glyph_t *
raqm_get_glyphs (raqm_t *rq,
                 size_t *length)
{
  size_t count = 0;

  if (!rq || !rq->runs || !length)
  {
    if (length)
      *length = 0;
    return NULL;
  }

  for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
    count += hb_buffer_get_length (run->buffer);

  *length = count;

  if (rq->glyphs)
    free (rq->glyphs);

  rq->glyphs = malloc (sizeof (raqm_glyph_t) * count);
  if (!rq->glyphs)
  {
    *length = 0;
    return NULL;
  }

  RAQM_TEST ("Glyph information:\n");

  count = 0;
  for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
  {
    size_t len;
    hb_glyph_info_t *info;
    hb_glyph_position_t *position;

    len = hb_buffer_get_length (run->buffer);
    info = hb_buffer_get_glyph_infos (run->buffer, NULL);
    position = hb_buffer_get_glyph_positions (run->buffer, NULL);

    for (size_t i = 0; i < len; i++)
    {
      rq->glyphs[count + i].index = info[i].codepoint;
      rq->glyphs[count + i].cluster = info[i].cluster;
      rq->glyphs[count + i].x_advance = position[i].x_advance;
      rq->glyphs[count + i].y_advance = position[i].y_advance;
      rq->glyphs[count + i].x_offset = position[i].x_offset;
      rq->glyphs[count + i].y_offset = position[i].y_offset;

      RAQM_TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
          rq->glyphs[count + i].index, rq->glyphs[count + i].x_offset,
          rq->glyphs[count + i].y_offset, rq->glyphs[count + i].x_advance);
    }

    count += len;
  }

  if (rq->flags & RAQM_FLAG_UTF8)
  {
#ifdef RAQM_TESTING
    RAQM_TEST ("\nUTF-32 clusters:");
    for (size_t i = 0; i < count; i++)
      RAQM_TEST (" %02d", rq->glyphs[i].cluster);
    RAQM_TEST ("\n");
#endif

    for (size_t i = 0; i < count; i++)
      rq->glyphs[i].cluster = _raqm_u32_to_u8_index (rq,
                                                     rq->glyphs[i].cluster);

#ifdef RAQM_TESTING
    RAQM_TEST ("UTF-8 clusters: ");
    for (size_t i = 0; i < count; i++)
      RAQM_TEST (" %02d", rq->glyphs[i].cluster);
    RAQM_TEST ("\n");
#endif
  }
  return rq->glyphs;
}

static bool
_raqm_resolve_scripts (raqm_t *rq);

static hb_direction_t
_raqm_hb_dir (raqm_t *rq, FriBidiLevel level)
{
  hb_direction_t dir = HB_DIRECTION_LTR;

  if (rq->base_dir == RAQM_DIRECTION_TTB)
      dir = HB_DIRECTION_TTB;
  else if (FRIBIDI_LEVEL_IS_RTL (level))
      dir = HB_DIRECTION_RTL;

  return dir;
}

static bool
_raqm_itemize (raqm_t *rq)
{
  FriBidiParType par_type = FRIBIDI_PAR_ON;
  FriBidiCharType types[rq->text_len];
  FriBidiLevel levels[rq->text_len];
  FriBidiRun *runs = NULL;
  raqm_run_t *last;
  int max_level;
  int run_count;
  bool ok = true;

#ifdef RAQM_TESTING
  switch (rq->base_dir)
  {
    case RAQM_DIRECTION_RTL:
      RAQM_TEST ("Direction is: RTL\n\n");
      break;
    case RAQM_DIRECTION_LTR:
      RAQM_TEST ("Direction is: LTR\n\n");
      break;
    case RAQM_DIRECTION_TTB:
      RAQM_TEST ("Direction is: TTB\n\n");
      break;
    case RAQM_DIRECTION_DEFAULT:
    default:
      RAQM_TEST ("Direction is: DEFAULT\n\n");
      break;
  }
#endif

  if (rq->base_dir == RAQM_DIRECTION_RTL)
    par_type = FRIBIDI_PAR_RTL;
  else if (rq->base_dir == RAQM_DIRECTION_LTR)
    par_type = FRIBIDI_PAR_LTR;

  if (rq->base_dir == RAQM_DIRECTION_TTB)
  {
    /* Treat every thing as LTR in vertical text */
    max_level = 0;
    memset (types, FRIBIDI_TYPE_LTR, rq->text_len);
    memset (levels, 0, rq->text_len);
  }
  else
  {
    fribidi_get_bidi_types (rq->text, rq->text_len, types);
    max_level = fribidi_get_par_embedding_levels (types, rq->text_len,
                                                  &par_type, levels);
  }

  if (max_level < 0)
    return false;

  if (!_raqm_resolve_scripts (rq))
    return false;

  /* Get the number of bidi runs */
  run_count = fribidi_reorder_runs (types, rq->text_len, par_type,
                                    levels, NULL);

  /* Populate bidi runs array */
  runs = malloc (sizeof (FriBidiRun) * (size_t)run_count);
  if (!runs)
    return false;

  run_count = fribidi_reorder_runs (types, rq->text_len, par_type,
                                    levels, runs);

#ifdef RAQM_TESTING
  RAQM_TEST ("Number of runs before script itemization: %d\n\n", run_count);

  RAQM_TEST ("Fribidi Runs:\n");
  for (int i = 0; i < run_count; i++)
  {
    RAQM_TEST ("run[%d]:\t start: %d\tlength: %d\tlevel: %d\n",
               i, runs[i].pos, runs[i].len, runs[i].level);
  }
  RAQM_TEST ("\n");
#endif

  last = NULL;
  for (int i = 0; i < run_count; i++)
  {
    raqm_run_t *run = calloc (1, sizeof (raqm_run_t));
    if (!run)
      return NULL;

    if (!rq->runs)
      rq->runs = run;

    if (last)
      last->next = run;

    run->direction = _raqm_hb_dir (rq, runs[i].level);

    if (HB_DIRECTION_IS_BACKWARD (run->direction))
    {
      run->pos = runs[i].pos + runs[i].len - 1;
      run->script = rq->scripts[run->pos];
      for (int j = runs[i].len - 1; j >= 0; j--)
      {
        hb_script_t script = rq->scripts[runs[i].pos + j];
        if (script != run->script)
        {
          raqm_run_t *newrun = calloc (1, sizeof (raqm_run_t));
          if (!newrun)
            return NULL;
          newrun->pos = runs[i].pos + j;
          newrun->len = 1;
          newrun->direction = _raqm_hb_dir (rq, runs[i].level);
          newrun->script = script;
          run->next = newrun;
          run = newrun;
        }
        else
        {
          run->len++;
          run->pos = runs[i].pos + j;
        }
      }
    }
    else
    {
      run->pos = runs[i].pos;
      run->script = rq->scripts[run->pos];
      for (int j = 0; j < runs[i].len; j++)
      {
        hb_script_t script = rq->scripts[runs[i].pos + j];
        if (script != run->script)
        {
          raqm_run_t *newrun = calloc (1, sizeof (raqm_run_t));
          if (!newrun)
            return NULL;
          newrun->pos = runs[i].pos + j;
          newrun->len = 1;
          newrun->direction = _raqm_hb_dir (rq, runs[i].level);
          newrun->script = script;
          run->next = newrun;
          run = newrun;
        }
        else
          run->len++;
      }
    }

    last = run;
    last->next = NULL;
  }

#ifdef RAQM_TESTING
  run_count = 0;
  for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
    run_count++;
  RAQM_TEST ("Number of runs after script itemization: %d\n\n", run_count);

  run_count = 0;
  RAQM_TEST ("Final Runs:\n");
  for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
  {
    SCRIPT_TO_STRING (run->script);
    RAQM_TEST ("run[%d]:\t start: %d\tlength: %d\tdirection: %s\tscript: %s\n",
               run_count++, run->pos, run->len,
               hb_direction_to_string (run->direction), buff);
  }
  RAQM_TEST ("\n");
#endif

  free (runs);

  return ok;
}

/* Stack to handle script detection */
typedef struct {
  size_t       capacity;
  size_t       size;
  int         *pair_index;
  hb_script_t *scripts;
} raqm_stack_t;

/* Special paired characters for script detection */
static size_t paired_len = 34;
static const FriBidiChar paired_chars[] =
{
  0x0028, 0x0029, /* ascii paired punctuation */
  0x003c, 0x003e,
  0x005b, 0x005d,
  0x007b, 0x007d,
  0x00ab, 0x00bb, /* guillemets */
  0x2018, 0x2019, /* general punctuation */
  0x201c, 0x201d,
  0x2039, 0x203a,
  0x3008, 0x3009, /* chinese paired punctuation */
  0x300a, 0x300b,
  0x300c, 0x300d,
  0x300e, 0x300f,
  0x3010, 0x3011,
  0x3014, 0x3015,
  0x3016, 0x3017,
  0x3018, 0x3019,
  0x301a, 0x301b
};

static void
_raqm_stack_free (raqm_stack_t *stack)
{
  free (stack->scripts);
  free (stack->pair_index);
  free (stack);
}

/* Stack handling functions */
static raqm_stack_t *
_raqm_stack_new (size_t max)
{
  raqm_stack_t *stack;
  stack = calloc (1, sizeof (raqm_stack_t));
  if (!stack)
    return NULL;

  stack->scripts = malloc (sizeof (hb_script_t) * max);
  if (!stack->scripts)
  {
    _raqm_stack_free (stack);
    return NULL;
  }

  stack->pair_index = malloc (sizeof (int) * max);
  if (!stack->pair_index)
  {
    _raqm_stack_free (stack);
    return NULL;
  }

  stack->size = 0;
  stack->capacity = max;

  return stack;
}

static bool
_raqm_stack_pop (raqm_stack_t *stack)
{
  if (!stack->size)
  {
    RAQM_DBG ("Stack is Empty\n");
    return false;
  }

  stack->size--;

  return true;
}

static hb_script_t
_raqm_stack_top (raqm_stack_t *stack)
{
  if (!stack->size)
  {
    RAQM_DBG ("Stack is Empty\n");
    return HB_SCRIPT_INVALID; /* XXX: check this */
  }

  return stack->scripts[stack->size];
}

static bool
_raqm_stack_push (raqm_stack_t      *stack,
            hb_script_t script,
            int         pi)
{
  if (stack->size == stack->capacity)
  {
    RAQM_DBG ("Stack is Full\n");
    return false;
  }

  stack->size++;
  stack->scripts[stack->size] = script;
  stack->pair_index[stack->size] = pi;

  return true;
}

static int
get_pair_index (const FriBidiChar ch)
{
  int lower = 0;
  int upper = paired_len - 1;

  while (lower <= upper)
  {
    int mid = (lower + upper) / 2;
    if (ch < paired_chars[mid])
      upper = mid - 1;
    else if (ch > paired_chars[mid])
      lower = mid + 1;
    else
      return mid;
  }

  return -1;
}

#define STACK_IS_EMPTY(script)     ((script)->size <= 0)
#define STACK_IS_NOT_EMPTY(script) (! STACK_IS_EMPTY(script))
#define IS_OPEN(pair_index)        (((pair_index) & 1) == 0)

/* Resolve the script for each character in the input string, if the character
 * script is common or inherited it takes the script of the character before it
 * except paired characters which we try to make them use the same script. We
 * then split the BiDi runs, if necessary, on script boundaries.
 */
static bool
_raqm_resolve_scripts (raqm_t *rq)
{
  int last_script_index = -1;
  int last_set_index = -1;
  hb_script_t last_script_value = HB_SCRIPT_INVALID;
  raqm_stack_t *stack = NULL;

  if (rq->scripts)
    return true;

  rq->scripts = malloc (sizeof (hb_script_t) * rq->text_len);
  if (!rq->scripts)
    return false;

  for (size_t i = 0; i < rq->text_len; ++i)
    rq->scripts[i] = hb_unicode_script (hb_unicode_funcs_get_default (),
                                        rq->text[i]);

#ifdef RAQM_TESTING
  RAQM_TEST ("Before script detection:\n");
  for (size_t i = 0; i < rq->text_len; ++i)
  {
    SCRIPT_TO_STRING (rq->scripts[i]);
    RAQM_TEST ("script for ch[%zu]\t%s\n", i, buff);
  }
  RAQM_TEST ("\n");
#endif

  stack = _raqm_stack_new (rq->text_len);
  if (!stack)
    return false;

  for (int i = 0; i < (int) rq->text_len; i++)
  {
    if (rq->scripts[i] == HB_SCRIPT_COMMON && last_script_index != -1)
    {
      int pair_index = get_pair_index (rq->text[i]);
      if (pair_index >= 0)
      {
        if (IS_OPEN (pair_index))
        {
          /* is a paired character */
          rq->scripts[i] = last_script_value;
          last_set_index = i;
          _raqm_stack_push (stack, rq->scripts[i], pair_index);
        }
        else
        {
          /* is a close paired character */
          /* find matching opening (by getting the last even index for current
           * odd index) */
          int pi = pair_index & ~1;
          while (STACK_IS_NOT_EMPTY (stack) &&
                 stack->pair_index[stack->size] != pi)
          {
            _raqm_stack_pop (stack);
          }
          if (STACK_IS_NOT_EMPTY (stack))
          {
            rq->scripts[i] = _raqm_stack_top (stack);
            last_script_value = rq->scripts[i];
            last_set_index = i;
          }
          else
          {
            rq->scripts[i] = last_script_value;
            last_set_index = i;
          }
        }
      }
      else
      {
        rq->scripts[i] = last_script_value;
        last_set_index = i;
      }
    }
    else if (rq->scripts[i] == HB_SCRIPT_INHERITED && last_script_index != -1)
    {
      rq->scripts[i] = last_script_value;
      last_set_index = i;
    }
    else
    {
      for (int j = last_set_index + 1; j < i; ++j)
        rq->scripts[j] = rq->scripts[i];
      last_script_value = rq->scripts[i];
      last_script_index = i;
      last_set_index = i;
    }
  }

#ifdef RAQM_TESTING
  RAQM_TEST ("After script detection:\n");
  for (size_t i = 0; i < rq->text_len; ++i)
  {
    SCRIPT_TO_STRING (rq->scripts[i]);
    RAQM_TEST ("script for ch[%zu]\t%s\n", i, buff);
  }
  RAQM_TEST ("\n");
#endif

  _raqm_stack_free (stack);

  return true;
}

static bool
_raqm_shape (raqm_t *rq)
{
  for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
  {
    run->buffer = hb_buffer_create ();

    hb_buffer_add_utf32 (run->buffer, rq->text, rq->text_len,
                         run->pos, run->len);
    hb_buffer_set_script (run->buffer, run->script);
    hb_buffer_set_language (run->buffer, hb_language_get_default ());
    hb_buffer_set_direction (run->buffer, run->direction);
    hb_shape_full (rq->font, run->buffer, rq->features, rq->features_len,
                   NULL);
  }

  return true;
}

/* Convert index from UTF-32 to UTF-8 */
static uint32_t
_raqm_u32_to_u8_index (raqm_t   *rq,
                       uint32_t  index)
{
  FriBidiStrIndex length;
  char output[(sizeof (uint32_t) * index) + 1];

  length = fribidi_unicode_to_charset (FRIBIDI_CHAR_SET_UTF8,
                                       rq->text,
                                       index,
                                       output);
  return length;
}
