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

#include "raqm.h"

#ifdef TESTING
#define SCRIPT_TO_STRING(script) \
    char buff[5]; \
    hb_tag_to_string (hb_script_to_iso15924_tag (script), buff); \
    buff[4] = '\0';
#endif

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

  for (i = 0; i < len/2; i++)
    {
      FriBidiRun temp = arr[i];
      arr[i] = arr[len - 1 - i];
      arr[len - 1 - i] = temp;
    }
}

/* Seperates and reorders runs via fribidi using bidi algorithm*/
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

 if
    (len == 0)
    {
      goto out;
    }

  assert (bidi_types);
  assert (embedding_levels);

  {
    FriBidiStrIndex i;

    /* L1. Reset the embedding levels of some chars:
       4. any sequence of white space characters at the end of the line. */
    for (i = len - 1; i >= 0 &&
         FRIBIDI_IS_EXPLICIT_OR_BN_OR_WS (bidi_types[i]); i--)
      embedding_levels[i] = FRIBIDI_DIR_TO_LEVEL (base_dir);
  }
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
    int capacity;
    int size;
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

    hb_buffer_t* hb_buffer;
    hb_script_t hb_script;
} Run;

/* Stack handeling functions */
static Stack*
stack_create (int max)
{
    Stack* stack;
    stack = (Stack*) malloc (sizeof (Stack));
    stack->scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * max);
    stack->pair_index = (int*) malloc (sizeof (int) * max);
    stack->size = 0;
    stack->capacity = max;
    return stack;
}

static int
stack_pop (Stack* stack)
{
    if (stack->size == 0)
    {
        DBG ("Stack is Empty\n");
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
        DBG ("Stack is Empty\n");
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
        DBG ("Stack is Full\n");
    }
    else
    {
        stack->size++;
        stack->scripts[stack->size] = script;
        stack->pair_index[stack->size] = pi;
    }

    return;
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

/* Seperates runs based on scripts */
static int
itemize_by_script(int bidirun_count,
                 hb_script_t *scripts,
                 FriBidiRun *bidiruns,
                 Run *runs)
{
    int i, j;
    int run_count = 0;

    /* To get number of runs after script seperation */
    for (i = 0; i < bidirun_count; i++)
    {
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
        return run_count;
    }

    run_count = 0;

    /* By iterating through bidiruns, any detection of different scripts in the same run
     * will split the run so that each run contains one script. Runs with odd levels but
     * different scripts will need to be reordered to appear in the correct visual order */
    for (i = 0; i < bidirun_count; i++)
    {
        FriBidiRun bidirun = bidiruns[i];

        runs[run_count].level = bidirun.level;
        runs[run_count].len = 0;

        if (!FRIBIDI_LEVEL_IS_RTL (bidirun.level))
        {
            runs[run_count].pos = bidirun.pos;
            runs[run_count].hb_script = scripts[bidirun.pos];
            for (j = 0; j < bidirun.len; j++)
            {
                hb_script_t script = scripts[bidirun.pos + j];
                if (script == runs[run_count].hb_script)
                {
                    runs[run_count].len++;
                }
                else
                {
                    run_count++;
                    runs[run_count].pos = bidirun.pos + j;
                    runs[run_count].level = bidirun.level;
                    runs[run_count].hb_script = script;
                    runs[run_count].len = 1;
                }
            }
        }
        else
        {
            runs[run_count].pos = bidirun.pos + bidirun.len -1;
            runs[run_count].hb_script = scripts[bidirun.pos + bidirun.len - 1];
            for (j = bidirun.len - 1; j >= 0; j--)
            {
                hb_script_t script = scripts[bidirun.pos + j];
                if (script == runs[run_count].hb_script)
                {
                    runs[run_count].len++;
                    runs[run_count].pos = bidirun.pos + j;
                }
                else
                {
                    run_count++;
                    runs[run_count].pos = bidirun.pos + j;
                    runs[run_count].level = bidirun.level;
                    runs[run_count].hb_script = script;
                    runs[run_count].len = 1;
                }
            }
        }
        run_count++;
    }

    return run_count;
}

/* Does the shaping for each run buffer */
static void
harfbuzz_shape (FriBidiChar* unicode_str,
                FriBidiStrIndex length,
                hb_font_t* hb_font,
                Run* run)
{
    run->hb_buffer = hb_buffer_create ();

    /* adding text to current buffer */
    hb_buffer_add_utf32 (run->hb_buffer, unicode_str, length, run->pos, run->len);

    /* setting script of current buffer */
    hb_buffer_set_script (run->hb_buffer, run->hb_script);

    /* setting language of current buffer */
    hb_buffer_set_language (run->hb_buffer, hb_language_get_default ());

    /* setting direction of current buffer */
    if (FRIBIDI_LEVEL_IS_RTL (run->level))
    {
        hb_buffer_set_direction (run->hb_buffer, HB_DIRECTION_RTL);
    }
    else
    {
        hb_buffer_set_direction (run->hb_buffer, HB_DIRECTION_LTR);
    }

    /* shaping current buffer */
    hb_shape (hb_font, run->hb_buffer, NULL, 0);
}

/* convert index from UTF-32 to UTF-8 */
static uint32_t
utf32_index_to_utf8 (FriBidiChar* unicode,
                     uint32_t index)
{
    FriBidiStrIndex length;
    char* output = (char*) malloc ((index * 4) + 1);

    length = fribidi_unicode_to_charset (FRIBIDI_CHAR_SET_UTF8, unicode, index, output);

    free (output);
    return length;
}

/* Takes the input text and does the reordering and shaping */
raqm_glyph_info_t*
raqm_shape (const char* text,
            FT_Face face,
            raqm_direction_t direction)
{
    int i = 0;
    const char* str;
    FriBidiStrIndex size;
    FriBidiChar* unicode_str;
    int index = 0;
    str = text;
    size = strlen (str);

    unicode_str = (FriBidiChar*) malloc (sizeof (FriBidiChar) * size);
    FriBidiStrIndex length = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8, str, size, unicode_str);
    raqm_glyph_info_t* glyph_info = raqm_shape_u32(unicode_str,face,direction);
    
        for (i = 0; glyph_info[i].index > -1 ; i++)
        {
           
            glyph_info[index].cluster = utf32_index_to_utf8 (unicode_str, glyph_info[i].cluster);
            TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
                  glyph_info[index].index, glyph_info[index].x_offset,
                  glyph_info[index].y_offset, glyph_info[index].x_advance);
            index++;
        }
    

    return glyph_info;
}

/* Takes a utf-32 input text and does the reordering and shaping */
raqm_glyph_info_t*
raqm_shape_u32 (unsigned int* unicode_str,
            FT_Face face,
            raqm_direction_t direction)
{
    int i = 0;
    const char* str;
    FriBidiCharType* types;
    FriBidiLevel* levels;
    FriBidiParType par_type;
    hb_script_t* scripts;
    hb_unicode_funcs_t* unicode_funcs;
    hb_script_t last_script_value;
    int last_script_index = -1;
    int last_set_index = -1;
    Stack* script_stack;
    int run_count;
    int bidirun_count;
    FriBidiRun* fribidi_runs;
    Run* runs;
    hb_font_t* hb_font;
    unsigned int glyph_count;
    int total_glyph_count = 0;
    unsigned int postion_length;
    hb_glyph_info_t* hb_glyph_info;
    hb_glyph_position_t* hb_glyph_position;
    raqm_glyph_info_t* glyph_info, *glyph_info_temp;
    int index = 0;
    
    FriBidiStrIndex length = 0;
    for(i=0; unicode_str[i];i++)
    {
        length++;
    }

    types = (FriBidiCharType*) malloc (sizeof (FriBidiCharType) * length);
    levels = (FriBidiLevel*) malloc (sizeof (FriBidiLevel) * length);
    fribidi_get_bidi_types (unicode_str, length, types);

    par_type = FRIBIDI_PAR_ON;
    if (direction == RAQM_DIRECTION_RTL)
    {
        par_type = FRIBIDI_PAR_RTL;
    }
    else if (direction == RAQM_DIRECTION_LTR)
    {
        par_type = FRIBIDI_PAR_LTR;
    }

    TEST ("Text is: %s\n\n", text);

    fribidi_get_par_embedding_levels (types, length, &par_type, levels);

    /* Handeling script detection for each character of the input string,
       if the character script is common or inherited it takes the script
       of the character before it except some special paired characters */
    scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * length);
    unicode_funcs = hb_unicode_funcs_get_default ();

    TEST ("Before script detection:\n");

    for (i = 0; i < length; ++i)
    {
        scripts[i] = hb_unicode_script (unicode_funcs, unicode_str[i]);
#ifdef TESTING
        SCRIPT_TO_STRING (scripts[i]);
        TEST ("script for ch[%d]\t%s\n", i, buff);
#endif
    }

    script_stack = stack_create (length);
    for (i = 0; i < length; ++i)
    {
        if (scripts[i] == HB_SCRIPT_COMMON && last_script_index != -1)
        {
            int pair_index = get_pair_index (unicode_str[i]);
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

#ifdef TESTING
    TEST ("\nAfter script detection:\n");
    for (i = 0; i < length; ++i)
    {
        SCRIPT_TO_STRING (scripts[i]);
        TEST ("script for ch[%d]\t%s\n", i, buff);
    }
#endif

    /* to get number of bidi runs */
    bidirun_count = fribidi_reorder_runs (types, length, par_type, levels, NULL);
    fribidi_runs = (FriBidiRun*) malloc (sizeof (FriBidiRun) * bidirun_count);

    /* to populate bidi run array */
    fribidi_reorder_runs (types, length, par_type, levels, fribidi_runs);

    TEST ("\nNumber of runs before script itemization: %d\n", bidirun_count);
    TEST ("\n");
    TEST ("Fribidi Runs:\n");
    for (i = 0; i < bidirun_count; ++i)
    {
        TEST ("run[%d]:\t start: %d\tlength: %d\tlevel: %d\n",
              i, fribidi_runs[i].pos, fribidi_runs[i].len,
              fribidi_runs[i].level);
    }
    TEST ("\n");

    /* to get number of runs after script seperation */
    run_count = itemize_by_script (bidirun_count, scripts, fribidi_runs, NULL);
    runs = (Run*) malloc (sizeof (Run) * run_count);

    /* to populate runs_scripts array */
    itemize_by_script (bidirun_count, scripts, fribidi_runs, runs);
    TEST ("Number of runs after script itemization: %d\n", run_count);

#ifdef TESTING
    TEST ("\n");
    TEST ("Final Runs:\n");
    for (i = 0; i < run_count; ++i)
    {
        SCRIPT_TO_STRING (runs[i].hb_script);
        TEST ("run[%d]:\t start: %d\tlength: %d\tlevel: %d\tscript: %s\n",
             i, runs[i].pos, runs[i].len, runs[i].level, buff);
    }
    TEST ("\n");
#endif

    /* harfbuzz shaping */
    hb_font = hb_ft_font_create (face, NULL);

    for (i = 0; i < run_count; i++)
    {
        harfbuzz_shape (unicode_str, length, hb_font, &runs[i]);
        hb_glyph_info = hb_buffer_get_glyph_infos (runs[i].hb_buffer, &glyph_count);
        total_glyph_count += glyph_count;
    }

    glyph_info = (raqm_glyph_info_t*) malloc (sizeof (raqm_glyph_info_t) * (total_glyph_count + 1));
    glyph_info_temp = (raqm_glyph_info_t*) malloc (sizeof (raqm_glyph_info_t) * (total_glyph_count + 1)); 

    TEST ("Glyph information:\n");

    for (i = 0; i < run_count; i++)
    {
        hb_glyph_info = hb_buffer_get_glyph_infos (runs[i].hb_buffer, &glyph_count);
        hb_glyph_position = hb_buffer_get_glyph_positions (runs[i].hb_buffer, &postion_length);
        int j;
        for (j = 0; j < glyph_count; j++)
        {
            glyph_info_temp[index].index = hb_glyph_info[j].codepoint;
            glyph_info_temp[index].x_offset = hb_glyph_position[j].x_offset;
            glyph_info_temp[index].y_offset = hb_glyph_position[j].y_offset;
            glyph_info_temp[index].x_advance = hb_glyph_position[j].x_advance;
            glyph_info_temp[index].cluster =  hb_glyph_info[j].cluster;
            TEST ("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
                  glyph_info_temp[index].index, glyph_info_temp[index].x_offset,
                  glyph_info_temp[index].y_offset, glyph_info_temp[index].x_advance);
            index++;
        }
    }
    glyph_info_temp[total_glyph_count].index = -1;
    glyph_info = glyph_info_temp;

    free (unicode_str);
    free (levels);
    free (scripts);
    free (types);
    free (fribidi_runs);
    free (runs);

    return glyph_info;
}
