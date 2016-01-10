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
 * The most recent version of this code can be found in:
 * https://github.com/HOST-Oman/libraqm
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <hb.h>
#include <hb-ft.h>

#include "raqm.h"
#include "reorder_runs.h"

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

typedef struct _raqm_run raqm_run_t;

struct _raqm {
  int ref_count;

  uint32_t *text;
  size_t text_len;

  raqm_direction_t base_dir;

  hb_script_t *scripts;
#ifdef RAQM_MULTI_FONT
  hb_font_t **fonts;
#else
  hb_font_t *font;
#endif

  raqm_run_t *runs;

};

struct _raqm_run
{
  FriBidiStrIndex pos;
  FriBidiStrIndex len;

  hb_direction_t direction;
  hb_script_t script;
  hb_font_t *font;
  hb_buffer_t *buffer;

  raqm_run_t *next;
};

/**
 * raqm_create:
 *
 * Creates a new #raqm_t with all its internal states initialized to their
 * defaults.
 *
 * Return value:
 * a newly allocated #raqm_t with a reference count of 1. The initial reference
 * count should be released with raqm_destroy() when you are done using the
 * #raqm_t.
 *
 * Since: 0.1
 */
raqm_t *
raqm_create (void)
{
  raqm_t *rq;

  rq = malloc (sizeof (raqm_t));
  rq->ref_count = 1;

  rq->text = NULL;
  rq->text_len = 0;

  rq->base_dir = RAQM_DIRECTION_DEFAULT;

  rq->scripts = NULL;

#ifdef RAQM_MULTI_FONT
  rq->fonts = NULL;
#else
  rq->font = NULL;
#endif

  rq->runs = NULL;

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
  if (rq != NULL)
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
    free (run);
  }
}

#ifdef RAQM_MULTI_FONT
static void
_raqm_free_fonts (raqm_t *rq)
{
  if (rq->fonts == NULL)
    return;

  for (size_t i = 0; i < rq->text_len; i++)
  {
    if (rq->fonts[i] != NULL)
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
  if (rq == NULL || --rq->ref_count != 0)
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
  free (rq);
}

/**
 * raqm_add_text:
 * @rq: a #raqm_t.
 * @text: a UTF-32 encoded text string.
 * @len: the length of @text.
 *
 * Adds @text to @rq to be used for layout. It must be a valid UTF-32 text, any
 * invalid character will be replaced with U+FFFD. The text should typically
 * represent a full paragraph, since doing the layout of chunks of text
 * separately can give improper output.
 *
 * Since: 0.1
 */
void
raqm_add_text (raqm_t         *rq,
               const uint32_t *text,
               size_t          len)
{
  if (rq == NULL)
    return;

  rq->text_len = len;
  rq->text = malloc (sizeof (uint32_t) * len);

  for (size_t i = 0; i < len; i++)
    rq->text[i] = text[i];
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
 * Since: 0.1
 */
void
raqm_set_par_direction (raqm_t          *rq,
                        raqm_direction_t dir)
{
  if (rq == NULL)
    return;

  rq->base_dir = dir;
}

/**
 * raqm_set_freetype_face:
 * @rq: a #raqm_t.
 * @face: an #FT_Face.
 * @start: index of first character that should use @face.
 * @len: number of characters using @face.
 *
 * Sets an #FT_Face to be used for @len-number of characters staring at @start.
 *
 * This method can be used repeatedly to set different faces for different
 * parts of the text. It is the responsibility of the client to make sure that
 * face ranges cover the whole text.
 *
 * Since: 0.1
 */
void
raqm_set_freetype_face (raqm_t *rq,
                        FT_Face face,
                        size_t  start,
                        size_t  len)
{
  if (rq == NULL || rq->text_len == 0 || start >= rq->text_len)
    return;

  if (start + len > rq->text_len)
    len = rq->text_len - start;

#ifdef RAQM_MULTI_FONT
  if (rq->fonts == NULL)
    rq->fonts = calloc (sizeof (intptr_t), rq->text_len);

  for (size_t i = 0; i < len; i++)
  {
    if (rq->fonts[start + i] != NULL)
      hb_font_destroy (rq->fonts[start + i]);
    rq->fonts[start + i] = hb_ft_font_create_referenced (face);
  }
#else
  if (rq->font != NULL)
    hb_font_destroy (rq->font);
#ifdef HAVE_HB_FT_FONT_CREATE_REFERENCED
  rq->font = hb_ft_font_create_referenced (face);
#else
  rq->font = hb_ft_font_create (face, NULL);
#endif /* HAVE_HB_FT_FONT_CREATE_REFERENCED */
#endif /* RAQM_MULTI_FONT */
}

static bool
_raqm_itemize (raqm_t *rq);

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
  if (rq == NULL || rq->text_len == 0)
    return false;

  if (!_raqm_itemize (rq))
    return false;

  return true;
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
  FriBidiCharType *types = NULL;
  FriBidiLevel *levels = NULL;
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

  types = malloc (sizeof (FriBidiCharType) * rq->text_len);
  levels = malloc (sizeof (FriBidiLevel) * rq->text_len);

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
    max_level = fribidi_get_par_embedding_levels (types, rq->text_len, &par_type, levels);
  }

  if (max_level < 0)
  {
    ok = false;
    goto out;
  }

  /* Get the number of bidi runs */
  run_count = fribidi_reorder_runs (types, rq->text_len, par_type, levels, NULL);

  /* Populate bidi runs array */
  runs = malloc (sizeof (FriBidiRun) * (size_t)run_count);
  run_count = fribidi_reorder_runs (types, rq->text_len, par_type, levels, runs);

  if (!_raqm_resolve_scripts (rq))
  {
    ok = false;
    goto out;
  }

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

    if (rq->runs == NULL)
      rq->runs = run;

    if (last != NULL)
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
  }

  last->next = NULL;

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
               run_count++, run->pos, run->len, hb_direction_to_string (run->direction), buff);
  }
  RAQM_TEST ("\n");
#endif

out:
  free (levels);
  free (types);
  free (runs);

  return ok;
}

/* Stack to handle script detection */
typedef struct
{
    size_t capacity;
    size_t size;
    int* pair_index;
    hb_script_t* scripts;
} Stack;

/* Special paired characters for script detection */
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

/* Stack handeling functions */
static Stack*
stack_new (size_t max)
{
    Stack* stack;
    stack = (Stack*) malloc (sizeof (Stack));
    stack->scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * max);
    stack->pair_index = (int*) malloc (sizeof (int) * max);
    stack->size = 0;
    stack->capacity = max;
    return stack;
}

static void
stack_free (Stack* stack)
{
    free (stack->scripts);
    free (stack->pair_index);
    free (stack);

}

static int
stack_pop (Stack* stack)
{
    if (stack->size == 0)
    {
        RAQM_DBG ("Stack is Empty\n");
        return 1;
    }
    else
    {
        stack->size--;
    }

    return 0;
}

static hb_script_t
stack_top (Stack* stack)
{
    if (stack->size == 0)
    {
        RAQM_DBG ("Stack is Empty\n");
    }

    return stack->scripts[stack->size];
}

static void
stack_push (Stack* stack,
            hb_script_t script,
            int pi)
{
    if (stack->size == stack->capacity)
    {
        RAQM_DBG ("Stack is Full\n");
    }
    else
    {
        stack->size++;
        stack->scripts[stack->size] = script;
        stack->pair_index[stack->size] = pi;
    }
}

static int
get_pair_index (const FriBidiChar firbidi_ch)
{
    int lower = 0;
    int upper = 33; /* paired characters array length */
    while (lower <= upper)
    {
        int mid = (lower + upper) / 2;
        if (firbidi_ch < paired_chars[mid])
        {
            upper = mid - 1;
        }
        else if (firbidi_ch > paired_chars[mid])
        {
            lower = mid + 1;
        }
        else
        {
            return mid;
        }
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
  Stack* stack = NULL;

  if (rq->scripts != NULL)
    return true;

  rq->scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * (size_t) rq->text_len);
  for (size_t i = 0; i < rq->text_len; ++i)
    rq->scripts[i] = hb_unicode_script (hb_unicode_funcs_get_default (), rq->text[i]);

#ifdef RAQM_TESTING
  RAQM_TEST ("Before script detection:\n");
  for (size_t i = 0; i < rq->text_len; ++i)
  {
    SCRIPT_TO_STRING (rq->scripts[i]);
    RAQM_TEST ("script for ch[%ld]\t%s\n", i, buff);
  }
  RAQM_TEST ("\n");
#endif

  stack = stack_new ((size_t) rq->text_len);
  for (int i = 0; i < (int) rq->text_len; ++i)
  {
    if (rq->scripts[i] == HB_SCRIPT_COMMON && last_script_index != -1)
    {
      int pair_index = get_pair_index (rq->text[i]);
      if (pair_index >= 0)
      {    /* is a paired character */
        if (IS_OPEN (pair_index))
        {
          rq->scripts[i] = last_script_value;
          last_set_index = i;
          stack_push (stack, rq->scripts[i], pair_index);
        }
        else
        {    /* is a close paired character */
          int pi = pair_index & ~1; /* find matching opening (by getting the last even index for currnt odd index)*/
          while (STACK_IS_NOT_EMPTY (stack) &&
                 stack->pair_index[stack->size] != pi)
          {
            stack_pop (stack);
          }
          if (STACK_IS_NOT_EMPTY (stack))
          {
            rq->scripts[i] = stack_top (stack);
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
      int j;
      for (j = last_set_index + 1; j < i; ++j)
      {
        rq->scripts[j] = rq->scripts[i];
      }
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
    RAQM_TEST ("script for ch[%ld]\t%s\n", i, buff);
  }
  RAQM_TEST ("\n");
#endif

  stack_free (stack);

  return true;
}

/* Does the shaping for each run buffer */
static void
harfbuzz_shape (const FriBidiChar* unicode_str,
                FriBidiStrIndex length,
                hb_font_t* hb_font,
                const char** featurelist,
                raqm_run_t* run)
{
    run->buffer = hb_buffer_create ();

    /* adding text to current buffer */
    hb_buffer_add_utf32 (run->buffer, unicode_str, length, (unsigned int)(run->pos), run->len);

    /* setting script of current buffer */
    hb_buffer_set_script (run->buffer, run->script);

    /* setting language of current buffer */
    hb_buffer_set_language (run->buffer, hb_language_get_default ());

    /* setting direction of current buffer */
    hb_buffer_set_direction (run->buffer, run->direction);

    /* shaping current buffer */
    if (featurelist)
    {
        unsigned int count = 0;
        const char** p;
        hb_feature_t* features = NULL;

        for (p = featurelist; *p; p++)
        {
            count++;
        }

        features = (hb_feature_t *) malloc (sizeof (hb_feature_t) * count);

        count = 0;
        for (p = featurelist; *p; p++)
        {
            hb_bool_t success =  hb_feature_from_string (*p, -1, &features[count]);
            if (success)
            {
                count++;
            }
        }
        hb_shape_full (hb_font, run->buffer, features, count, NULL);
    }
    else
    {
        hb_shape (hb_font, run->buffer, NULL, 0);
    }
}

/* convert index from UTF-32 to UTF-8 */
static uint32_t
u32_index_to_u8 (FriBidiChar* unicode,
                 uint32_t index)
{
    FriBidiStrIndex length;
    char* output = (char*) malloc ((size_t)((index * 4) + 1));

    length = fribidi_unicode_to_charset (FRIBIDI_CHAR_SET_UTF8, unicode, (FriBidiStrIndex)(index), output);

    free (output);
    return (uint32_t)(length);
}

/* Takes the input text and does the reordering and shaping */
unsigned int
raqm_shape (const char* u8_str,
            int u8_size,
            const FT_Face face,
            raqm_direction_t direction,
            const char **features,
            raqm_glyph_info_t** glyph_info)
{
    FriBidiChar* u32_str;
    FriBidiStrIndex u32_size;
    raqm_glyph_info_t* info;
    unsigned int glyph_count;
    unsigned int i;

    RAQM_TEST ("Text is: %s\n", u8_str);

    u32_str = (FriBidiChar*) calloc (sizeof (FriBidiChar), (size_t)(u8_size));
    u32_size = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8, u8_str, u8_size, u32_str);

    glyph_count = raqm_shape_u32 (u32_str, u32_size, face, direction, features, &info);

#ifdef RAQM_TESTING
    RAQM_TEST ("\nUTF-32 clusters:");
    for (i = 0; i < glyph_count; i++)
    {
        RAQM_TEST (" %02d", info[i].cluster);
    }
    RAQM_TEST ("\n");
#endif

    for (i = 0; i < glyph_count; i++)
    {
        info[i].cluster = u32_index_to_u8 (u32_str, info[i].cluster);
    }

#ifdef RAQM_TESTING
    RAQM_TEST ("UTF-8 clusters: ");
    for (i = 0; i < glyph_count; i++)
    {
        RAQM_TEST (" %02d", info[i].cluster);
    }
    RAQM_TEST ("\n");
#endif

    free (u32_str);
    *glyph_info = info;
    return glyph_count;
}

/* Takes a utf-32 input text and does the reordering and shaping */
unsigned int
raqm_shape_u32 (const uint32_t* text,
                int length,
                const FT_Face face,
                raqm_direction_t direction,
                const char **features,
                raqm_glyph_info_t** glyph_info)
{
    unsigned int index = 0;
    unsigned int total_glyph_count = 0;
    unsigned int glyph_count;
    unsigned int postion_length;

    hb_font_t* hb_font = NULL;
    hb_glyph_info_t* hb_glyph_info = NULL;
    hb_glyph_position_t* hb_glyph_position = NULL;
    raqm_glyph_info_t* info = NULL;
    raqm_t *rq;

    rq = raqm_create ();
    raqm_add_text (rq, text, length);
    raqm_set_par_direction (rq, direction);
    raqm_set_freetype_face (rq, face, 0, length);
    raqm_set_freetype_face (rq, face, 0, length);

    if (!raqm_layout (rq))
      goto out;

    /* harfbuzz shaping */
    hb_font = hb_ft_font_create (face, NULL);

    for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
    {
        harfbuzz_shape (text, length, hb_font, features, run);
        hb_glyph_info = hb_buffer_get_glyph_infos (run->buffer, &glyph_count);
        total_glyph_count += glyph_count;
    }

    info = (raqm_glyph_info_t*) malloc (sizeof (raqm_glyph_info_t) * (total_glyph_count));

    RAQM_TEST ("Glyph information:\n");

    for (raqm_run_t *run = rq->runs; run != NULL; run = run->next)
    {
        hb_glyph_info = hb_buffer_get_glyph_infos (run->buffer, &glyph_count);
        hb_glyph_position = hb_buffer_get_glyph_positions (run->buffer, &postion_length);

        for (size_t i = 0; i < glyph_count; i++)
        {
            info[index].index = hb_glyph_info[i].codepoint;
            info[index].x_offset = hb_glyph_position[i].x_offset;
            info[index].y_offset = hb_glyph_position[i].y_offset;
            info[index].x_advance = hb_glyph_position[i].x_advance;
            info[index].y_advance = hb_glyph_position[i].y_advance;
            info[index].cluster = hb_glyph_info[i].cluster;
            RAQM_TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
                  info[index].index, info[index].x_offset,
                  info[index].y_offset, info[index].x_advance);
            index++;
        }
        hb_buffer_destroy (run->buffer);
    }

out:

    *glyph_info = info;

    hb_font_destroy (hb_font);
    raqm_destroy (rq);

    return total_glyph_count;
}
