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

#include <assert.h>
#include <string.h>
#include "raqm.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wundef"
#include <fribidi.h>
#pragma GCC diagnostic pop
#include <hb.h>
#include <hb-ft.h>

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

struct _raqm {
  int ref_count;

  uint32_t *text;
  size_t text_len;
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
raqm_add_text (raqm_t   *rq,
               uint32_t *text,
               size_t    len)
{
  if (rq == NULL)
    return;

  rq->text_len = len;
  rq->text = malloc (sizeof (uint32_t) * len);

  for (size_t i = 0; i < len; i++)
    rq->text[i] = text[i];
}

/* for older fribidi versions */
#ifndef HAVE_FRIBIDI_REORDER_RUNS
typedef struct _FriBidiRunStruct FriBidiRun;
struct _FriBidiRunStruct
{
  FriBidiRun *prev;
  FriBidiRun *next;

  FriBidiStrIndex pos, len;
  FriBidiCharType type;
  FriBidiLevel level;
};

/* Reverse an array of runs */
static void reverse_run (FriBidiRun *arr, const FriBidiStrIndex len)
{
  FriBidiStrIndex i;

  assert (arr);

  for (i = 0; i < len / 2; i++)
    {
      FriBidiRun temp = arr[i];
      arr[i] = arr[len - 1 - i];
      arr[len - 1 - i] = temp;
    }
}

/* Seperates and reorders runs via fribidi using bidi algorithm*/
static
FriBidiStrIndex fribidi_reorder_runs (
  /* input */
  const FriBidiCharType *bidi_types,
  const FriBidiStrIndex len,
  const FriBidiParType base_dir,
  /* input and output */
  FriBidiLevel *embedding_levels,
  /* output */
  FriBidiRun *runs
)
{
  FriBidiStrIndex i;
  FriBidiLevel level;
  FriBidiLevel last_level = -1;
  FriBidiLevel max_level = 0;
  FriBidiStrIndex run_count = 0;
  FriBidiStrIndex run_start = 0;
  FriBidiStrIndex run_index = 0;

  if (len == 0)
    {
      goto out;
    }

  assert (bidi_types);
  assert (embedding_levels);

  /* L1. Reset the embedding levels of some chars:
     4. any sequence of white space characters at the end of the line. */
  for (i = len - 1; i >= 0 &&
       FRIBIDI_IS_EXPLICIT_OR_BN_OR_WS (bidi_types[i]); i--)
     embedding_levels[i] = FRIBIDI_DIR_TO_LEVEL (base_dir);

  /* Find max_level of the line.  We don't reuse the paragraph
   * max_level, both for a cleaner API, and that the line max_level
   * may be far less than paragraph max_level. */
  for (i = len - 1; i >= 0; i--)
    if (embedding_levels[i] > max_level)
       max_level = embedding_levels[i];

  for (i = 0; i < len; i++)
    {
      if (embedding_levels[i] != last_level)
        run_count++;

      last_level = embedding_levels[i];
    }

  if (runs == NULL)
    goto out;

  while (run_start < len)
    {
      int runLength = 0;
      while ((run_start + runLength) < len && embedding_levels[run_start] == embedding_levels[run_start + runLength])
        runLength++;

      runs[run_index].pos = run_start;
      runs[run_index].level = embedding_levels[run_start];
      runs[run_index].len = runLength;
      run_start += runLength;
      run_index++;
    }

  /* L2. Reorder. */
  for (level = max_level; level > 0; level--)
  {
      for (i = run_count - 1; i >= 0; i--)
      {
          if (runs[i].level >= level)
          {
              int end = i;
              for (i--; (i >= 0 && runs[i].level >= level); i--)
                  ;
              reverse_run (runs + i + 1, end - i);
          }
      }
  }

out:

  return run_count;
}

#endif

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

typedef struct
{
    FriBidiStrIndex pos, len;
    FriBidiCharType type;
    FriBidiLevel level;

    hb_direction_t direction;
    hb_buffer_t* buffer;
    hb_script_t script;
} Run;

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

/* Detecting script for each character in the input string, if the character
 * script is common or inherited it takes the script of the character before it
 * except paired characters which we try to make them use the same script. We
 * then split the BiDi runs, if necessary, on script boundaries.
 */
static int
itemize_by_script(int bidirun_count,
                 FriBidiRun *bidiruns,
                 const uint32_t* text,
                 int length,
                 Run *runs)
{
    int i;
    int run_count = 0;
    int last_script_index = -1;
    int last_set_index = -1;
    hb_script_t last_script_value = HB_SCRIPT_INVALID;
    hb_script_t* scripts = NULL;
    Stack* script_stack = NULL;

    scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * (size_t) length);
    for (i = 0; i < length; ++i)
    {
        scripts[i] = hb_unicode_script (hb_unicode_funcs_get_default (), text[i]);
    }

#ifdef RAQM_TESTING
    if (runs)
    {
        RAQM_TEST ("Before script detection:\n");
        for (i = 0; i < length; ++i)
        {
            SCRIPT_TO_STRING (scripts[i]);
            RAQM_TEST ("script for ch[%d]\t%s\n", i, buff);
        }
        RAQM_TEST ("\n");
    }
#endif

    script_stack = stack_new ((size_t) length);
    for (i = 0; i < length; ++i)
    {
        if (scripts[i] == HB_SCRIPT_COMMON && last_script_index != -1)
        {
            int pair_index = get_pair_index (text[i]);
            if (pair_index >= 0)
            {    /* is a paired character */
                if (IS_OPEN (pair_index))
                {
                    scripts[i] = last_script_value;
                    last_set_index = i;
                    stack_push (script_stack, scripts[i], pair_index);
                }
                else
                {        /* is a close paired character */
                    int pi = pair_index & ~1; /* find matching opening (by getting the last even index for currnt odd index)*/
                    while (STACK_IS_NOT_EMPTY (script_stack) &&
                           script_stack->pair_index[script_stack->size] != pi)
                    {
                        stack_pop (script_stack);
                    }
                    if (STACK_IS_NOT_EMPTY (script_stack))
                    {
                        scripts[i] = stack_top (script_stack);
                        last_script_value = scripts[i];
                        last_set_index = i;
                    }
                    else
                    {
                        scripts[i] = last_script_value;
                        last_set_index = i;
                    }
                }
            }
            else
            {
                scripts[i] = last_script_value;
                last_set_index = i;
            }
        }
        else if (scripts[i] == HB_SCRIPT_INHERITED && last_script_index != -1)
        {
            scripts[i] = last_script_value;
            last_set_index = i;
        }
        else
        {
            int j;
            for (j = last_set_index + 1; j < i; ++j)
            {
                scripts[j] = scripts[i];
            }
            last_script_value = scripts[i];
            last_script_index = i;
            last_set_index = i;
        }
    }

#ifdef RAQM_TESTING
    if (runs)
    {
        RAQM_TEST ("After script detection:\n");
        for (i = 0; i < length; ++i)
        {
            SCRIPT_TO_STRING (scripts[i]);
            RAQM_TEST ("script for ch[%d]\t%s\n", i, buff);
        }
        RAQM_TEST ("\n");

        RAQM_TEST ("Number of runs before script itemization: %d\n", bidirun_count);
        RAQM_TEST ("\n");
        RAQM_TEST ("Fribidi Runs:\n");
        for (i = 0; i < bidirun_count; ++i)
        {
            RAQM_TEST ("run[%d]:\t start: %d\tlength: %d\tlevel: %d\n",
                  i, bidiruns[i].pos, bidiruns[i].len, bidiruns[i].level);
        }
        RAQM_TEST ("\n");
    }
#endif


    /* To get number of runs after script seperation */
    for (i = 0; i < bidirun_count; i++)
    {
        int j;
        FriBidiRun run = bidiruns[i];
        hb_script_t last_script = scripts[run.pos];
        run_count++;
        for (j = 0; j < run.len; j++) {
            hb_script_t script = scripts[run.pos + j];
            if (script != last_script)
            {
                run_count++;
            }
            last_script = script;
        }
    }

    if (runs == NULL)
    {
        goto out;
    }

    run_count = 0;

    /* By iterating through bidiruns, any detection of different scripts in the same run
     * will split the run so that each run contains one script. Runs with odd levels but
     * different scripts will need to be reordered to appear in the correct visual order */
    for (i = 0; i < bidirun_count; i++)
    {
        int j;
        FriBidiRun bidirun = bidiruns[i];

        runs[run_count].level = bidirun.level;
        runs[run_count].len = 0;

        if (!FRIBIDI_LEVEL_IS_RTL (bidirun.level))
        {
            runs[run_count].pos = bidirun.pos;
            runs[run_count].script = scripts[bidirun.pos];
            for (j = 0; j < bidirun.len; j++)
            {
                hb_script_t script = scripts[bidirun.pos + j];
                if (script == runs[run_count].script)
                {
                    runs[run_count].len++;
                }
                else
                {
                    run_count++;
                    runs[run_count].pos = bidirun.pos + j;
                    runs[run_count].level = bidirun.level;
                    runs[run_count].script = script;
                    runs[run_count].len = 1;
                }
            }
        }
        else
        {
            runs[run_count].pos = bidirun.pos + bidirun.len -1;
            runs[run_count].script = scripts[bidirun.pos + bidirun.len - 1];
            for (j = bidirun.len - 1; j >= 0; j--)
            {
                hb_script_t script = scripts[bidirun.pos + j];
                if (script == runs[run_count].script)
                {
                    runs[run_count].len++;
                    runs[run_count].pos = bidirun.pos + j;
                }
                else
                {
                    run_count++;
                    runs[run_count].pos = bidirun.pos + j;
                    runs[run_count].level = bidirun.level;
                    runs[run_count].script = script;
                    runs[run_count].len = 1;
                }
            }
        }
        run_count++;
    }

#ifdef RAQM_TESTING
    RAQM_TEST ("Number of runs after script itemization: %d\n", run_count);
    RAQM_TEST ("\n");
    RAQM_TEST ("Final Runs:\n");
    for (i = 0; i < run_count; ++i)
    {
        SCRIPT_TO_STRING (runs[i].script);
        RAQM_TEST ("run[%d]:\t start: %d\tlength: %d\tlevel: %d\tscript: %s\n",
              i, runs[i].pos, runs[i].len, runs[i].level, buff);
    }
    RAQM_TEST ("\n");
#endif

out:

    stack_free(script_stack);
    free (scripts);
    return run_count;
}

/* Does the shaping for each run buffer */
static void
harfbuzz_shape (const FriBidiChar* unicode_str,
                FriBidiStrIndex length,
                hb_font_t* hb_font,
                const char** featurelist,
                Run* run)
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
    int i = 0;
    unsigned int index = 0;
    int run_count;
    int bidirun_count;
    unsigned int total_glyph_count = 0;
    unsigned int glyph_count;
    unsigned int postion_length;
    int max_level;

    hb_font_t* hb_font = NULL;
    hb_glyph_info_t* hb_glyph_info = NULL;
    hb_glyph_position_t* hb_glyph_position = NULL;
    FriBidiParType par_type;
    FriBidiRun* bidiruns = NULL;
    FriBidiCharType* types = NULL;
    FriBidiLevel* levels = NULL;
    Run* runs = NULL;
    raqm_glyph_info_t* info = NULL;

    types = (FriBidiCharType*) malloc (sizeof (FriBidiCharType) * (size_t)(length));
    levels = (FriBidiLevel*) malloc (sizeof (FriBidiLevel) * (size_t)(length));

    par_type = FRIBIDI_PAR_ON;
    if (direction == RAQM_DIRECTION_RTL)
    {
        par_type = FRIBIDI_PAR_RTL;
    }
    else if (direction == RAQM_DIRECTION_LTR)
    {
        par_type = FRIBIDI_PAR_LTR;
    }

#ifdef RAQM_TESTING
    switch (direction)
    {
        case RAQM_DIRECTION_RTL:
            RAQM_TEST ("Direction is: RTL\n\n");
            break;
        case RAQM_DIRECTION_LTR:
            RAQM_TEST ("Direction is: LTR\n\n");
            break;
        case RAQM_DIRECTION_DEFAULT:
            RAQM_TEST ("Direction is: DEFAULT\n\n");
            break;
        case RAQM_DIRECTION_TTB:
            RAQM_TEST ("Direction is: TTB\n\n");
            break;
        default:
            RAQM_TEST ("Direction is: DEFAULT\n\n");
            break;
    }
#endif

    if (direction == RAQM_DIRECTION_TTB)
    {
        max_level = 0;
        memset(types, FRIBIDI_TYPE_LTR, (size_t)length);
        memset(levels, 0, (size_t)length);
    }
    else
    {
        fribidi_get_bidi_types (text, length, types);
        max_level = fribidi_get_par_embedding_levels (types, length, &par_type, levels);
    }

    if (max_level < 0)
        goto out;

    /* to get number of bidi runs */
    bidirun_count = fribidi_reorder_runs (types, length, par_type, levels, NULL);
    bidiruns = (FriBidiRun*) malloc (sizeof (FriBidiRun) * (size_t)(bidirun_count));

    /* to populate bidi run array */
    bidirun_count = fribidi_reorder_runs (types, length, par_type, levels, bidiruns);

    /* to get number of runs after script seperation */
    run_count = itemize_by_script (bidirun_count, bidiruns, text, length, NULL);
    runs = (Run*) malloc (sizeof (Run) * (size_t)(run_count));

    /* to populate runs_scripts array */
    itemize_by_script (bidirun_count, bidiruns, text, length, runs);

    /* harfbuzz shaping */
    hb_font = hb_ft_font_create (face, NULL);

    for (i = 0; i < run_count; i++)
    {
        /* setting direction of current buffer */
        runs[i].direction = HB_DIRECTION_LTR;
        if (direction == RAQM_DIRECTION_TTB)
        {
            runs[i].direction = HB_DIRECTION_TTB;
        }
        else if (FRIBIDI_LEVEL_IS_RTL (runs[i].level))
        {
            runs[i].direction = HB_DIRECTION_RTL;
        }
        harfbuzz_shape (text, length, hb_font, features, &runs[i]);
        hb_glyph_info = hb_buffer_get_glyph_infos (runs[i].buffer, &glyph_count);
        total_glyph_count += glyph_count;
    }

    info = (raqm_glyph_info_t*) malloc (sizeof (raqm_glyph_info_t) * (total_glyph_count));

    RAQM_TEST ("Glyph information:\n");

    for (i = 0; i < run_count; i++)
    {
        unsigned int j;
        hb_glyph_info = hb_buffer_get_glyph_infos (runs[i].buffer, &glyph_count);
        hb_glyph_position = hb_buffer_get_glyph_positions (runs[i].buffer, &postion_length);

        for (j = 0; j < glyph_count; j++)
        {
            info[index].index = hb_glyph_info[j].codepoint;
            info[index].x_offset = hb_glyph_position[j].x_offset;
            info[index].y_offset = hb_glyph_position[j].y_offset;
            info[index].x_advance = hb_glyph_position[j].x_advance;
            info[index].y_advance = hb_glyph_position[j].y_advance;
            info[index].cluster = hb_glyph_info[j].cluster;
            RAQM_TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
                  info[index].index, info[index].x_offset,
                  info[index].y_offset, info[index].x_advance);
            index++;
        }
        hb_buffer_destroy (runs[i].buffer);
    }

out:

    *glyph_info = info;

    hb_font_destroy (hb_font);
    free (levels);
    free (types);
    free (bidiruns);
    free (runs);

    return total_glyph_count;
}
