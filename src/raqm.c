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

#include <ucdn.h>

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
 * #include "raqm.h"
 *
 * int
 * main (int argc, char *argv[])
 * {
 *     const char *fontfile;
 *     const char *text;
 *     const char *direction;
 *     int i;
 *     int ret = 1;
 *
 *     raqm_t *rq = NULL;
 *     raqm_glyph_t *glyphs = NULL;
 *     size_t count;
 *     raqm_direction_t dir;
 *
 *     FT_Library ft_library = NULL;
 *     FT_Face face = NULL;
 *     FT_Error ft_error;
 *
 *
 *     if (argc < 4)
 *     {
 *         printf ("Usage:\n FONT_FILE TEXT DIRECTION\n");
 *         return 1;
 *     }
 *
 *     fontfile =  argv[1];
 *     text = argv[2];
 *     direction = argv[3];
 *
 *     ft_error = FT_Init_FreeType (&ft_library);
 *     if (ft_error)
 *         goto final;
 *     ft_error = FT_New_Face (ft_library, fontfile, 0, &face);
 *     if (ft_error)
 *         goto final;
 *     ft_error = FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0);
 *     if (ft_error)
 *         goto final;
 *
 *     dir = RAQM_DIRECTION_DEFAULT;
 *     if (strcmp(direction, "-rtl") == 0)
 *       dir = RAQM_DIRECTION_RTL;
 *     else if (strcmp(direction, "-ltr") == 0)
 *       dir = RAQM_DIRECTION_LTR;
 *
 *     rq = raqm_create ();
 *     if (rq == NULL)
 *         goto final;
 *
 *     if (!raqm_set_text_utf8 (rq, text, strlen (text)))
 *         goto final;
 *
 *     if (!raqm_set_freetype_face (rq, face))
 *         goto final;
 *
 *     if (!raqm_set_par_direction (rq, dir))
 *         goto final;
 *
 *     if (!raqm_layout (rq))
 *         goto final;
 *
 *     glyphs = raqm_get_glyphs (rq, &count);
 *     if (glyphs == NULL)
 *         goto final;
 *
 *     for (i = 0; i < count; i++)
 *     {
 *         printf("%d %d %d %d %d %d\n",
 *                glyphs[i].index,
 *                glyphs[i].x_offset,
 *                glyphs[i].y_offset,
 *                glyphs[i].x_advance,
 *                glyphs[i].y_advance,
 *                glyphs[i].cluster);
 *     }
 *
 *     ret = 0;
 *
 * final:
 *     raqm_destroy (rq);
 *     FT_Done_Face (face);
 *     FT_Done_FreeType (ft_library);
 *
 *     return ret;
 * }
 * ]|
 * To compile this example:
 * |[<prompt>
 * cc -o test test.c `pkg-config --libs --cflags raqm`
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

typedef enum {
  RAQM_FLAG_NONE = 0,
  RAQM_FLAG_UTF8 = 1 << 0
} raqm_flags_t;

typedef struct _raqm_run raqm_run_t;

struct _raqm {
  int              ref_count;

  uint32_t        *text;
  char            *text_utf8;
  size_t           text_len;

  raqm_direction_t base_dir;
  raqm_direction_t resolved_dir;

  hb_feature_t    *features;
  size_t           features_len;

  hb_script_t     *scripts;

  FT_Face         *ftfaces;

  raqm_run_t      *runs;
  raqm_glyph_t    *glyphs;

  raqm_flags_t     flags;

  int              line_width;
  raqm_alignment_t alignment;
};

struct _raqm_run {
  int            pos;
  int            len;
  int			 line;

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
  rq->text_utf8 = NULL;
  rq->text_len = 0;

  rq->base_dir = RAQM_DIRECTION_DEFAULT;
  rq->resolved_dir = RAQM_DIRECTION_DEFAULT;

  rq->features = NULL;
  rq->features_len = 0;

  rq->scripts = NULL;

  rq->ftfaces = NULL;

  rq->runs = NULL;
  rq->glyphs = NULL;

  rq->flags = RAQM_FLAG_NONE;

  rq->line_width = 2147483647;

  rq->alignment = RAQM_ALIGNMENT_LEFT;

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


static void
_raqm_free_ftfaces (raqm_t *rq)
{
  if (!rq->ftfaces)
    return;
  for (size_t i = 0; i < rq->text_len; i++)
  {
    if (rq->ftfaces[i])
      FT_Done_Face(rq->ftfaces[i]);
  }
}


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
  _raqm_free_ftfaces (rq);
  free (rq->ftfaces);
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
    return false;
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
#ifdef _MSC_VER
  uint32_t *unicode = _alloca (len);
#else
  uint32_t unicode[len];
#endif
  size_t ulen;

  if (!rq || !text || !len)
    return false;

  RAQM_TEST ("Text is: %s\n", text);

  rq->flags |= RAQM_FLAG_UTF8;

  rq->text_utf8 = malloc (sizeof (char) * strlen (text));
  if (!rq->text_utf8)
    return false;

  memcpy (rq->text_utf8, text, sizeof (char) * strlen (text));

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

  if (!rq->ftfaces)
    rq->ftfaces = calloc (sizeof (intptr_t), rq->text_len);
  if (!rq->ftfaces)
    return false;

  for (size_t i = 0; i < len; i++)
  {
    if (rq->ftfaces[start + i]) {
        FT_Done_Face (rq->ftfaces[start + i]);
    }
    rq->ftfaces[start + i] = face;
    FT_Reference_Face (face);
  }

  return true;
}

/**
 * raqm_set_line_width:
 * @rq: a #raqm_t.
 * @width: the line width.
 *
 * Sets the maximum line width for the paragraph, so that when the width is
 * exceeded, a breaking line mechanism is applied.
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.2
 */
bool
raqm_set_line_width (raqm_t *rq, int width)
{
  if (!rq)
    return false;

  rq->line_width = width;

  return true;
}

/**
 * raqm_set_paragraph_alignment:
 * @rq: a #raqm_t.
 * @alignment: paragraph alignment.
 *
 * Sets the paragraph alignment.
 *
 * Return value:
 * %true if no errors happened, %false otherwise.
 *
 * Since: 0.2
 */
bool
raqm_set_paragraph_alignment (raqm_t *rq, raqm_alignment_t alignment)
{
  if (!rq)
    return false;

  rq->alignment = alignment;

  return true;
}

static bool
_raqm_itemize (raqm_t *rq);

static bool
_raqm_shape (raqm_t *rq);

static bool
_raqm_line_break (raqm_t *rq);

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
  if (!rq || !rq->text_len || !rq->ftfaces)
    return false;

  for (size_t i = 0; i < rq->text_len; i++)
  {
      if (!rq->ftfaces[i])
          return false;
  }

  if (!_raqm_itemize (rq))
    return false;

  if (!_raqm_shape (rq))
    return false;

  if (!_raqm_line_break (rq))
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

  if (!rq->glyphs)
  {
    *length = 0;
    return NULL;
  }

  RAQM_TEST ("Glyph information:\n");

  for (size_t i = 0; i < *length; i++)
  {
      RAQM_TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\tx_position:"
                 " %d\ty_position: %d\tfont: %s\n",
                 rq->glyphs[i].index, rq->glyphs[i].x_offset,
                 rq->glyphs[i].y_offset, rq->glyphs[i].x_advance,
                 rq->glyphs[i].x_position, rq->glyphs[i].y_position,
                 rq->glyphs[i].ftface->family_name);
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

enum break_class
{
	// input types
	OP = 0,	// open
	CL,	// closing punctuation
	CP,	// closing parentheses (from 5.2.0) (before 5.2.0 treat like CL)
	QU,	// quotation
	GL,	// glue
	NS,	// no-start
	EX,	// exclamation/interrogation
	SY,	// Syntax (slash)
	IS,	// infix (numeric) separator
	PR,	// prefix
	PO,	// postfix
	NU,	// numeric
	AL,	// alphabetic
	ID,	// ideograph (atomic)
	IN,	// inseparable
	HY,	// hyphen
	BA,	// break after
	BB,	// break before
	B2,	// break both
	ZW,	// ZW space
	CM,	// combining mark
	WJ, // word joiner

	// used for Korean Syllable Block pair table
	H2, // Hamgul 2 Jamo Syllable
	H3, // Hangul 3 Jamo Syllable
	JL, // Jamo leading consonant
	JV, // Jamo vowel
	JT, // Jamo trailing consonant

	// these are not handled in the pair tables
	SA, // South (East) Asian
	SP,	// space
	PS,	// paragraph and line separators
	BK,	// hard break (newline)
	CR, // carriage return
	LF, // line feed
	NL, // next line
	CB, // contingent break opportunity
	SG, // surrogate
	AI, // ambiguous
	XX, // unknown
};

// Define some short-cuts for the table
#define oo DIRECT_BREAK				// '_' break allowed
#define SS INDIRECT_BREAK				// '%' only break across space (aka 'indirect break' below)
#define cc COMBINING_INDIRECT_BREAK	// '#' indirect break for combining marks
#define CC COMBINING_PROHIBITED_BREAK	// '@' indirect break for combining marks
#define XX PROHIBITED_BREAK			// '^' no break allowed_BRK

enum break_action {
        DIRECT_BREAK = 0,             	// _ in table, 	oo in array
        INDIRECT_BREAK,               	// % in table, 	SS in array
        COMBINING_INDIRECT_BREAK,		// # in table, 	cc in array
        COMBINING_PROHIBITED_BREAK,  	// @ in table 	CC in array
        PROHIBITED_BREAK,             	// ^ in table, 	XX in array
};

static bool *
_raqm_find_line_break (raqm_t *rq)
{
	size_t length = rq->text_len;
	enum break_class   current_class;
	enum break_class   next_class;
	enum break_action *break_actions;
	enum break_action  current_action;
	bool *break_here;

	enum break_action  break_pairs[][JT+1] =  {
		//       1   2   3   4	5	6	7	8	9  10  11  12  13  14  15  16  17  18  19  20  21   22  23  24  25  26  27
		//     OP, CL, CL, QU, GL, NS, EX, SY, IS, PR, PO, NU, AL, ID, IN, HY, BA, BB, B2, ZW, CM, WJ,  H2, H3, JL, JV, JT, = after class
		/*OP*/ { XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, CC, XX, XX, XX, XX, XX, XX }, // OP open
		/*CL*/ { oo, XX, XX, SS, SS, XX, XX, XX, XX, SS, SS, oo, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // CL close
		/*CP*/ { oo, XX, XX, SS, SS, XX, XX, XX, XX, SS, SS, SS, SS, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // CL close
		/*QU*/ { XX, XX, XX, SS, SS, SS, XX, XX, XX, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, XX, cc, XX, SS, SS, SS, SS, SS }, // QU quotation
		/*GL*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, XX, cc, XX, SS, SS, SS, SS, SS }, // GL glue
		/*NS*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, oo, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // NS no-start
		/*EX*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, oo, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // EX exclamation/interrogation
		/*SY*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // SY Syntax (slash)
		/*IS*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, SS, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // IS infix (numeric) separator
		/*PR*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, SS, SS, oo, SS, SS, oo, oo, XX, cc, XX, SS, SS, SS, SS, SS }, // PR prefix
		/*PO*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, SS, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // NU numeric
		/*NU*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, SS, SS, SS, SS, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // AL alphabetic
		/*AL*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, SS, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // AL alphabetic
		/*ID*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // ID ideograph (atomic)
		/*IN*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // IN inseparable
		/*HY*/ { oo, XX, XX, SS, oo, SS, XX, XX, XX, oo, oo, SS, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // HY hyphens and spaces
		/*BA*/ { oo, XX, XX, SS, oo, SS, XX, XX, XX, oo, oo, oo, oo, oo, oo, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // BA break after
		/*BB*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, XX, cc, XX, SS, SS, SS, SS, SS }, // BB break before
		/*B2*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, oo, oo, oo, oo, SS, SS, oo, XX, XX, cc, XX, oo, oo, oo, oo, oo }, // B2 break either side, but not pair
		/*ZW*/ { oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, oo, XX, oo, oo, oo, oo, oo, oo, oo }, // ZW zero width space
		/*CM*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, oo, SS, SS, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, oo }, // CM combining mark
		/*WJ*/ { SS, XX, XX, SS, SS, SS, XX, XX, XX, SS, SS, SS, SS, SS, SS, SS, SS, SS, SS, XX, cc, XX, SS, SS, SS, SS, SS }, // WJ word joiner
		/*H2*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, SS, SS }, // Hangul 2 Jamo syllable
		/*H3*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, SS }, // Hangul 3 Jamo syllable
		/*JL*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, SS, SS, SS, SS, oo }, // Jamo Leading Consonant
		/*JV*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, SS, SS }, // Jamo Vowel
		/*JT*/ { oo, XX, XX, SS, SS, SS, XX, XX, XX, oo, SS, oo, oo, oo, SS, SS, SS, oo, oo, XX, cc, XX, oo, oo, oo, oo, SS }, // Jamo Trailing Consonant
	};

	break_actions = malloc (sizeof (enum break_action) * length);
	break_here = malloc (sizeof (bool) * length);

	current_class = ucdn_get_resolved_linebreak_class(rq->text[0]);
	next_class    = ucdn_get_resolved_linebreak_class(rq->text[1]);

	// handle case where input starts with an LF
	if (current_class == LF)
		current_class = BK;

	// treat NL like BK
	if (current_class == NL)
		current_class = BK;

	// treat SP at start of input as if it followed WJ
	if (current_class == SP)
		current_class = WJ;

	// loop over all pairs in the string up to a hard break or CRLF pair
	for (size_t i = 1; (i < length) && (current_class != BK) && (current_class != CR || next_class == LF); i++)
	{
		next_class = ucdn_get_resolved_linebreak_class(rq->text[i]);

		// handle spaces explicitly
		if (next_class == SP) {
                        break_actions[i-1]  = PROHIBITED_BREAK;   // apply rule LB 7: � SP
			continue;
		}

		if (next_class == BK || next_class == NL || next_class == LF) {
                        break_actions[i-1]  = PROHIBITED_BREAK;
			current_class = BK;
			continue;
		}

		if (next_class == CR)
		{
                        break_actions[i-1]  = PROHIBITED_BREAK;
			current_class = CR;
			continue;
		}

		// lookup pair table information in brkPairs[before, after];
		current_action = break_pairs[current_class][next_class];
		break_actions[i-1] = current_action;

                if (current_action == INDIRECT_BREAK)		// resolve indirect break
		{
			if (ucdn_get_resolved_linebreak_class(rq->text[i-1]) == SP)                    // if context is A SP * B
                                break_actions[i-1] = INDIRECT_BREAK;             //       break opportunity
			else                                        // else
                                break_actions[i-1] = PROHIBITED_BREAK;           //       no break opportunity
		}

                else if (current_action == COMBINING_PROHIBITED_BREAK)		// this is the case OP SP* CM
		{
                        break_actions[i-1] = COMBINING_PROHIBITED_BREAK;     // no break allowed
			if (ucdn_get_resolved_linebreak_class(rq->text[i-1]) != SP)
				continue;                               // apply rule 9: X CM* -> X
		}

                else if (current_action == COMBINING_INDIRECT_BREAK)		// resolve combining mark break
		{
                        break_actions[i-1] = PROHIBITED_BREAK;               // don't break before CM
			if (ucdn_get_resolved_linebreak_class(rq->text[i-1]) == SP)
			{
                                break_actions[i-1] = PROHIBITED_BREAK;		// legacy: keep SP CM together
				if (i > 1)
                                        break_actions[i-2] = ((ucdn_get_resolved_linebreak_class(rq->text[i-2]) == SP) ? INDIRECT_BREAK : DIRECT_BREAK);
			} else                                     // apply rule 9: X CM * -> X
				continue;
		}

		current_class = next_class;
	}

	for(size_t i = 0; i < length; i++)
	{
                if (break_actions[i] == INDIRECT_BREAK || break_actions[i] == DIRECT_BREAK )
		{
			break_here[i] = true;
		}
		else
			break_here[i] = false;
	}

	free (break_actions);
	return break_here;
}

static int
_raqm_logical_sort(const void *a,const void *b)
{
    raqm_glyph_t *ia = (raqm_glyph_t *)a;
    raqm_glyph_t *ib = (raqm_glyph_t *)b;

return ia->cluster - ib->cluster;
}

static int
_raqm_visual_sort(const void *a,const void *b)
{
    raqm_glyph_t *ia = (raqm_glyph_t *)a;
    raqm_glyph_t *ib = (raqm_glyph_t *)b;

    if (ia->line == ib->line)
    {
        return ia->visual_index - ib->visual_index;
    }

    else
    {
        return ia->line - ib->line;
    }
}

static bool
_raqm_line_break (raqm_t *rq)
{
    int count = 0;
    int glyphs_length = 0;
    int current_x = 0;
    int line_space;
    int current_line = 0;
    int align_offset = 0;

    int i = 0;
    int j = 0;
    int k = 0;

    int space_count = 0;
    int space_extension;

    bool *break_here = NULL;

    /* finding possible breaks in text */
    break_here = _raqm_find_line_break(rq);

    /* counting total glyphs */
    for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
      count += hb_buffer_get_length (run->buffer);

    glyphs_length = count;

    rq->glyphs = malloc (sizeof (raqm_glyph_t) * count);
    if (!rq->glyphs)
    {
      return false;
    }

    /* populating glyphs */
    count = 0;
    for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
    {
      size_t len;
      hb_glyph_info_t *info;
      hb_glyph_position_t *position;

      len = hb_buffer_get_length (run->buffer);
      info = hb_buffer_get_glyph_infos (run->buffer, NULL);
      position = hb_buffer_get_glyph_positions (run->buffer, NULL);

          for (i = 0; i < (int)len; i++)
          {
              rq->glyphs[count + i].index = info[i].codepoint;
              rq->glyphs[count + i].cluster = info[i].cluster;
              rq->glyphs[count + i].x_advance = position[i].x_advance;
              rq->glyphs[count + i].y_advance = position[i].y_advance;
              rq->glyphs[count + i].x_offset = position[i].x_offset;
              rq->glyphs[count + i].y_offset = position[i].y_offset;
              rq->glyphs[count + i].ftface = rq->ftfaces[rq->glyphs[count + i].cluster];
              rq->glyphs[count + i].visual_index = count + i;
              rq->glyphs[count + i].line = 0;
          }

      count += len;
    }

    /* Sorting glyphs to logical order */
    qsort(rq->glyphs, glyphs_length, sizeof(raqm_glyph_t), _raqm_logical_sort);

    /* Line breaking */
    current_x = 0;
    current_line = 0;
    for (i = 0; i < glyphs_length; i++)
    {
        rq->glyphs[i].line = current_line;
        current_x += rq->glyphs[i].x_offset + rq->glyphs[i].x_advance;

        if (current_x > rq->line_width)
        {
            while (!break_here[rq->glyphs[i].cluster] && i >= 0)
            {
                i--;
            }

            /* Next line cannot start with a white space */
            if (rq->text[rq->glyphs[i + 1].cluster] == 32)
            {
                for (j = i + 1; rq->text[rq->glyphs[j].cluster] == 32; j++)
                {
                    rq->glyphs[j].line = current_line;
                }
                i = j - 1; //skip those
            }

            /* Handeling glyphs for the same character */
            if (rq->glyphs[i + 1].cluster == rq->glyphs[i].cluster)
            {
                for (k = i + 1; rq->glyphs[k].cluster == rq->glyphs[i].cluster; k++)
                {
                    rq->glyphs[j].line = current_line;
                }
                i = k - 1; //skip those
            }

            current_line ++;
            current_x = 0;
        }
    }

    /* Sorting glyphs back to visual order */
    qsort(rq->glyphs, glyphs_length, sizeof(raqm_glyph_t), _raqm_visual_sort);

    /* calculating positions */
    current_line = 0;
    current_x = 0;
    for (i = 0; i < glyphs_length; i++)
    {
        line_space =  (-1) * (rq->ftfaces[rq->glyphs[i].cluster]->ascender + abs(rq->ftfaces[rq->glyphs[i].cluster]->descender));

        if (rq->glyphs[i].line != current_line)
        {   
            current_x = 0;
            current_line = rq->glyphs[i].line;
        }

        rq->glyphs[i].x_position = current_x + rq->glyphs[i].x_offset;
        rq->glyphs[i].y_position = rq->glyphs[i].y_offset + rq->glyphs[i].line * line_space;

        current_x += rq->glyphs[i].x_advance;
    }

    /* handeling alighnment */
    if (rq->alignment != RAQM_ALIGNMENT_LEFT)
    {
        if (rq->alignment == RAQM_ALIGNMENT_RIGHT)
        {
            current_line = -1;
            for (i = glyphs_length - 1; i >= 0; i--)
            {
                if (rq->glyphs[i].line != current_line)
                {
                    current_line = rq->glyphs[i].line;
                    align_offset = rq->line_width - (rq->glyphs[i].x_position + rq->glyphs[i].x_advance);
                    for (j = i; j >= 0 && rq->glyphs[j].line == current_line; j--)
                    {
                        rq->glyphs[j].x_position += align_offset;
                    }
                }
                i = j + 1;
            }
        }

        if (rq->alignment == RAQM_ALIGNMENT_CENTER)
        {
            current_line = -1;
            for (i = glyphs_length - 1; i >= 0; i--)
            {
                if (rq->glyphs[i].line != current_line)
                {
                    current_line = rq->glyphs[i].line;
                    align_offset = (rq->line_width - (rq->glyphs[i].x_position + rq->glyphs[i].x_advance)) / 2;
                    for (j = i; j >= 0 && rq->glyphs[j].line == current_line; j--)
                    {
                        rq->glyphs[j].x_position += align_offset;
                    }
                }
                i = j + 1;
            }
        }    
        if (rq->alignment == RAQM_ALIGNMENT_FULL)
        {
            current_line = -1;
            for (i = glyphs_length - 1; i >= 0; i--)
            {
                if (rq->glyphs[i].line != current_line)
                {
                    current_line = rq->glyphs[i].line;
                    align_offset = rq->line_width - (rq->glyphs[i].x_position + rq->glyphs[i].x_advance);

                    /* counting spaces in one line */
                    for (j = i; j >= 0 && rq->glyphs[j].line == current_line; j--)
                    {
                        if (rq->text[rq->glyphs[j].cluster] == 32) //space
                            space_count++;
                    }

                    /* distributing align offset to all spaces */
                    if (space_count == 0)
                        align_offset = 0;
                    else
                        align_offset = align_offset / space_count;
                    space_extension = 0;
                    for (k = j + 1; rq->glyphs[k].line == current_line; k++)
                    {
                        rq->glyphs[k].x_position += space_extension;
                        if (rq->text[rq->glyphs[k].cluster] == 32) //space
                        {
                            space_extension += align_offset;
                        }
                    }
                }
                i = j + 1;
            }
        }
    }

    free (break_here);
    return true;
}

static bool
_raqm_itemize (raqm_t *rq)
{
  FriBidiParType par_type = FRIBIDI_PAR_ON;
#ifdef _MSC_VER
  FriBidiCharType *types = _alloca (rq->text_len);
  FriBidiLevel *levels = _alloca (rq->text_len);
#else
  FriBidiCharType types[rq->text_len];
  FriBidiLevel levels[rq->text_len];
#endif
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
    rq->resolved_dir = RAQM_DIRECTION_LTR;
  }
  else
  {
    fribidi_get_bidi_types (rq->text, rq->text_len, types);
    max_level = fribidi_get_par_embedding_levels (types, rq->text_len,
                                                  &par_type, levels);

   if (par_type == FRIBIDI_PAR_LTR)
     rq->resolved_dir = RAQM_DIRECTION_LTR;
   else
     rq->resolved_dir = RAQM_DIRECTION_RTL;
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
      return false;

    if (!rq->runs)
      rq->runs = run;

    if (last)
      last->next = run;

    run->direction = _raqm_hb_dir (rq, runs[i].level);

    if (HB_DIRECTION_IS_BACKWARD (run->direction))
    {
      run->pos = runs[i].pos + runs[i].len - 1;
      run->script = rq->scripts[run->pos];
      run->font = HB_FT_FONT_CREATE (rq->ftfaces[run->pos]);
      for (int j = runs[i].len - 1; j >= 0; j--)
      {
        hb_script_t script = rq->scripts[runs[i].pos + j];
        FT_Face face = rq->ftfaces[runs[i].pos + j];
        if (script != run->script || face != rq->ftfaces[run->pos])
        {
          raqm_run_t *newrun = calloc (1, sizeof (raqm_run_t));
          if (!newrun)
            return false;
          newrun->pos = runs[i].pos + j;
          newrun->len = 1;
          newrun->direction = _raqm_hb_dir (rq, runs[i].level);
          newrun->script = script;
          newrun->font = HB_FT_FONT_CREATE (face);
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
      run->font = HB_FT_FONT_CREATE (rq->ftfaces[run->pos]);
      for (int j = 0; j < runs[i].len; j++)
      {
        hb_script_t script = rq->scripts[runs[i].pos + j];
        FT_Face face = rq->ftfaces[runs[i].pos + j];
        if (script != run->script || face != rq->ftfaces[run->pos])
        {
          raqm_run_t *newrun = calloc (1, sizeof (raqm_run_t));
          if (!newrun)
            return false;
          newrun->pos = runs[i].pos + j;
          newrun->len = 1;
          newrun->direction = _raqm_hb_dir (rq, runs[i].level);
          newrun->script = script;
          newrun->font = HB_FT_FONT_CREATE (face);
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
    RAQM_TEST ("run[%d]:\t start: %d\tlength: %d\tdirection: %s\tscript: %s\tfont: %s\n",
               run_count++, run->pos, run->len,
               hb_direction_to_string (run->direction), buff,
               rq->ftfaces[run->pos]->family_name);
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
    hb_shape_full (run->font, run->buffer, rq->features, rq->features_len,
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
#ifdef _MSC_VER
  char *output = _alloca ((sizeof (uint32_t) * index) + 1);
#else
  char output[(sizeof (uint32_t) * index) + 1];
#endif

  length = fribidi_unicode_to_charset (FRIBIDI_CHAR_SET_UTF8,
                                       rq->text,
                                       index,
                                       output);
  return length;
}

/* Convert index from UTF-8 to UTF-32 */
static uint32_t
_raqm_u8_to_u32_index (raqm_t   *rq,
                       uint32_t  index)
{
  FriBidiStrIndex length;
#ifdef _MSC_VER
  uint32_t *output = _alloca ((sizeof (uint32_t) * index) + 1);
#else
  uint32_t output[(sizeof (uint32_t) * index) + 1];
#endif

  length = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8,
                                       rq->text_utf8,
                                       index,
                                       output);
  return length;
}

static bool
_raqm_allowed_grapheme_boundary (hb_codepoint_t l_char,
                                hb_codepoint_t r_char);

static bool
_raqm_in_hangul_syllable (hb_codepoint_t ch);

/**
 * raqm_index_to_position:
 * @rq: a #raqm_t.
 * @index: (inout): character index.
 * @x: (out): output x position.
 * @y: (out): output y position.
 *
 * Calculates the cursor position after the character at @index. If the character
 * is right-to-left, then the cursor will be at the left of it, whereas if the
 * character is left-to-right, then the cursor will be at the right of it.
 *
 * Return value:
 * %true if the process was successful, %false otherwise.
 *
 * Since: 0.1
 */
bool
raqm_index_to_position (raqm_t *rq,
                        size_t *index,
                        int *x,
                        int *y)
{
  bool found = false;

  /* We don't currently support multiline, so y is always 0 */
  *y = 0;
  *x = 0;

  if (rq == NULL)
    return false;

  if (rq->flags & RAQM_FLAG_UTF8)
    *index = _raqm_u8_to_u32_index (rq, *index);

  if (*index >= rq->text_len)
    return false;

  RAQM_TEST ("\n");

  while(*index < rq->text_len)
  {
    if (_raqm_allowed_grapheme_boundary(rq->text[*index], rq->text[*index+1]))
      break;

    ++*index;
  }

  for (raqm_run_t *run = rq->runs; run != NULL && !found; run = run->next)
  {
    size_t len;
    hb_glyph_info_t *info;
    hb_glyph_position_t *position;
    len = hb_buffer_get_length (run->buffer);
    info = hb_buffer_get_glyph_infos (run->buffer, NULL);
    position = hb_buffer_get_glyph_positions (run->buffer, NULL);

    for (size_t i = 0; i < len && !found; i++)
    {
      uint32_t curr_cluster = info[i].cluster;
      uint32_t next_cluster = curr_cluster;
      *x += position[i].x_advance;

      if (run->direction == HB_DIRECTION_LTR)
        for (size_t j = i + 1; j < len && next_cluster == curr_cluster; j++)
          next_cluster = info[j].cluster;
      else
	for (int j = i - 1; i != 0 && j >= 0 && next_cluster == curr_cluster;
	     j--)
          next_cluster = info[j].cluster;

      if (next_cluster == curr_cluster)
        next_cluster = run->pos + run->len;

      if (*index < next_cluster && *index >= curr_cluster)
      {
        if (run->direction == HB_DIRECTION_RTL)
          *x -= position[i].x_advance;
        *index = curr_cluster;
        found = true;
      }
    }
  }

  if (rq->flags & RAQM_FLAG_UTF8)
    *index = _raqm_u32_to_u8_index(rq, *index);
  RAQM_TEST ("The position is %d at index %ld\n",*x ,*index);
  return true;
}

/**
 * raqm_position_to_index:
 * @rq: a #raqm_t.
 * @x: x position.
 * @y: y position.
 * @index: (out): output character index.
 *
 * Returns the @index of the character at @x and @y position within text.
 * If the position is outside the text, the last character is chosen as
 * @index.
 *
 * Return value:
 * %true if the process was successful, %false in case of error.
 *
 * Since: 0.1
 */
bool
raqm_position_to_index (raqm_t *rq,
                        int x,
                        int y,
                        size_t *index)
{
  int delta_x = 0, current_x = 0;
  (void)y;

  if (rq == NULL)
    return false;

  if (x < 0) /* Get leftmost index */
  {
    if (rq->resolved_dir == RAQM_DIRECTION_RTL)
      *index = rq->text_len;
    else
      *index = 0;
    return true;
  }

  RAQM_TEST ("\n");

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
      delta_x = position[i].x_advance;
      if (x < (current_x + delta_x))
      {
        bool before = false;
        if (run->direction == HB_DIRECTION_LTR)
          before = (x < current_x + (delta_x / 2));
        else
          before = (x > current_x + (delta_x / 2));

        if (before)
          *index = info[i].cluster;
        else
        {
          uint32_t curr_cluster = info[i].cluster;
          uint32_t next_cluster = curr_cluster;
          if (run->direction == HB_DIRECTION_LTR)
            for (size_t j = i + 1; j < len && next_cluster == curr_cluster; j++)
              next_cluster = info[j].cluster;
          else
	  for (int j = i - 1; i != 0 && j >= 0 && next_cluster == curr_cluster;
		 j--)
              next_cluster = info[j].cluster;

          if (next_cluster == curr_cluster)
            next_cluster = run->pos + run->len;

          *index = next_cluster;
        }
        if (_raqm_allowed_grapheme_boundary(rq->text[*index],rq->text[*index+1]))
        {
          RAQM_TEST ("The start-index is %ld  at position %d \n", *index, x);
            return true;
        }

        while (*index < (unsigned)run->pos + run->len)
        {
	  if (_raqm_allowed_grapheme_boundary(rq->text[*index],
					      rq->text[*index+1]))
          {
            *index += 1;
            break;
          }
          *index += 1;
        }
        RAQM_TEST ("The start-index is %ld  at position %d \n", *index, x);
        return true;
      }
      else
        current_x += delta_x;
    }
  }

  /* Get rightmost index*/
  if (rq->resolved_dir == RAQM_DIRECTION_RTL)
    *index = 0;
  else
    *index = rq->text_len;

  RAQM_TEST ("The start-index is %ld  at position %d \n", *index, x);

  return true;
}

typedef enum
{
  RAQM_GRAPHEM_CR,
  RAQM_GRAPHEM_LF,
  RAQM_GRAPHEM_CONTROL,
  RAQM_GRAPHEM_EXTEND,
  RAQM_GRAPHEM_REGIONAL_INDICATOR,
  RAQM_GRAPHEM_PREPEND,
  RAQM_GRAPHEM_SPACING_MARK,
  RAQM_GRAPHEM_HANGUL_SYLLABLE,
  RAQM_GRAPHEM_OTHER
} raqm_grapheme_t;

static raqm_grapheme_t
_raqm_get_grapheme_break (hb_codepoint_t ch,
                          hb_unicode_general_category_t category);

static bool
_raqm_allowed_grapheme_boundary (hb_codepoint_t l_char,
                                 hb_codepoint_t r_char)
{
  hb_unicode_general_category_t l_category;
  hb_unicode_general_category_t r_category;
  raqm_grapheme_t l_grapheme, r_grapheme;

  l_category = hb_unicode_general_category (hb_unicode_funcs_get_default (),
					    l_char);
  r_category = hb_unicode_general_category (hb_unicode_funcs_get_default (),
					    r_char);
  l_grapheme = _raqm_get_grapheme_break (l_char, l_category);
  r_grapheme = _raqm_get_grapheme_break (r_char, r_category);

  if (l_grapheme == RAQM_GRAPHEM_CR && r_grapheme == RAQM_GRAPHEM_LF)
    return false; /*Do not break between a CR and LF GB3*/
  if (l_grapheme == RAQM_GRAPHEM_CONTROL || l_grapheme == RAQM_GRAPHEM_CR ||
      l_grapheme == RAQM_GRAPHEM_LF || r_grapheme == RAQM_GRAPHEM_CONTROL ||
      r_grapheme == RAQM_GRAPHEM_CR || r_grapheme == RAQM_GRAPHEM_LF)
    return true; /*Break before and after CONTROL GB4, GB5*/
  if (r_grapheme == RAQM_GRAPHEM_HANGUL_SYLLABLE)
    return false; /*Do not break Hangul syllable sequences. GB6, GB7, GB8*/
  if (l_grapheme == RAQM_GRAPHEM_REGIONAL_INDICATOR &&
      r_grapheme == RAQM_GRAPHEM_REGIONAL_INDICATOR)
    return false; /*Do not break between regional indicator symbols. GB8a*/
  if (r_grapheme == RAQM_GRAPHEM_EXTEND)
    return false; /*Do not break before extending characters. GB9*/
  /*Do not break before SpacingMarks, or after Prepend characters.GB9a, GB9b*/
  if (l_grapheme == RAQM_GRAPHEM_PREPEND)
    return false;
  if (r_grapheme == RAQM_GRAPHEM_SPACING_MARK)
    return false;
  return true; /*Otherwise, break everywhere. GB1, GB2, GB10*/
}

static raqm_grapheme_t
_raqm_get_grapheme_break (hb_codepoint_t ch,
                          hb_unicode_general_category_t category)
{
  raqm_grapheme_t gb_type;

  gb_type = RAQM_GRAPHEM_OTHER;
  switch ((int)category)
  {
    case HB_UNICODE_GENERAL_CATEGORY_FORMAT:
      if (ch == 0x200C || ch == 0x200D)
        gb_type = RAQM_GRAPHEM_EXTEND;
      else
        gb_type = RAQM_GRAPHEM_CONTROL;
      break;

    case HB_UNICODE_GENERAL_CATEGORY_CONTROL:
      if (ch == 0x000D)
        gb_type = RAQM_GRAPHEM_CR;
      else if (ch == 0x000A)
        gb_type = RAQM_GRAPHEM_LF;
      else
        gb_type = RAQM_GRAPHEM_CONTROL;
      break;

    case HB_UNICODE_GENERAL_CATEGORY_SURROGATE:
    case HB_UNICODE_GENERAL_CATEGORY_LINE_SEPARATOR:
    case HB_UNICODE_GENERAL_CATEGORY_PARAGRAPH_SEPARATOR:
    case HB_UNICODE_GENERAL_CATEGORY_UNASSIGNED:
      if ((ch >= 0xFFF0 && ch <= 0xFFF8) ||
          (ch >= 0xE0000 && ch <= 0xE0FFF))
        gb_type = RAQM_GRAPHEM_CONTROL;
      break;

    case HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK:
    case HB_UNICODE_GENERAL_CATEGORY_ENCLOSING_MARK:
    case HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK:
      if (ch != 0x102B || ch != 0x102C || ch != 0x1038 ||
          (ch <= 0x1062 && ch >= 0x1064) || (ch <= 0x1067 && ch >= 0x106D) ||
          ch != 0x1083 || (ch <= 0x1087 && ch >= 0x108C) || ch != 0x108F ||
          (ch <= 0x109A && ch >= 0x109C) || ch != 0x1A61 || ch != 0x1A63 ||
          ch != 0x1A64 || ch != 0xAA7B || ch != 0xAA70 || ch != 0x11720 ||
          ch != 0x11721) /**/
        gb_type = RAQM_GRAPHEM_SPACING_MARK;

      else if (ch == 0x09BE || ch == 0x09D7 ||
          ch == 0x0B3E || ch == 0x0B57 || ch == 0x0BBE || ch == 0x0BD7 ||
          ch == 0x0CC2 || ch == 0x0CD5 || ch == 0x0CD6 ||
          ch == 0x0D3E || ch == 0x0D57 || ch == 0x0DCF || ch == 0x0DDF ||
          ch == 0x1D165 || (ch >= 0x1D16E && ch <= 0x1D172))
        gb_type = RAQM_GRAPHEM_EXTEND;
      break;

    case HB_UNICODE_GENERAL_CATEGORY_OTHER_LETTER:
      if (ch == 0x0E33 || ch == 0x0EB3)
        gb_type = RAQM_GRAPHEM_SPACING_MARK;
      break;

    case HB_UNICODE_GENERAL_CATEGORY_OTHER_SYMBOL:
      if (ch >= 0x1F1E6 && ch <= 0x1F1FF)
        gb_type = RAQM_GRAPHEM_REGIONAL_INDICATOR;
      break;

    default:
      gb_type = RAQM_GRAPHEM_OTHER;
      break;
  }

  if (_raqm_in_hangul_syllable (ch))
    gb_type = RAQM_GRAPHEM_HANGUL_SYLLABLE;

  return gb_type;
}

static bool
_raqm_in_hangul_syllable (hb_codepoint_t ch)
{
  (void)ch;
  return false;
}
