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

/* Stack to handle script detection */
typedef struct _Stack {
	int capacity;
	int size;
	int *pi;
	hb_script_t *scripts;

} Stack;

/* Special paired characters for script detection */
static const FriBidiChar paired_chars[] = {
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

/** ==========================WILL BE CHANGED */
typedef struct _Run {
    int start;
    int length;
    FriBidiLevel level;
    hb_buffer_t *hb_buffer;
    hb_script_t hb_script;
} Run;

/* Stack handeling functions */
Stack *create(int max) {
	Stack *S;
	S = (Stack *)malloc(sizeof(Stack));
	S->scripts = (hb_script_t *)malloc(sizeof(hb_script_t)*max);
	S->pi = (int *)malloc(sizeof(int)*max);
	S->size = 0;
	S->capacity = max;
	return S;
}
int pop(Stack *S) {
	if(S->size == 0) {
		DBG("Stack is Empty\n");
		return 1;
	}
	else
		S->size--;

	return 0;
}
hb_script_t top(Stack *S) {
	if(S->size == 0) {
		DBG("Stack is Empty\n");
	}
	return S->scripts[S->size-1];
}
void push(Stack *S, hb_script_t script, int pi) {
	if(S->size == S->capacity) {
		DBG("Stack is Full\n");
	}
	else {
		S->scripts[S->size++] = script;
		S->pi[S->size++] = pi;
	}
	return;
}
static int get_pair_index(const FriBidiChar ch) {
    int lower = 0;
    int upper = 33; /* paired characters array length */
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
#define STACK_IS_EMPTY(script) ((script)->size <= 0)
#define STACK_IS_NOT_EMPTY(script) (! STACK_IS_EMPTY(script))
#define IS_OPEN(pair_index) (((pair_index) & 1) == 0)

/* Reverses an array of runs used in shape_text */
static void reverse_run(Run* run, int len) {
    int i;
    for (i = 0; i < len/2; i++) {
	Run temp = run[i];
	run[i]=run[len - 1 - i];
	run[len - 1 - i] = temp;
    }
}

/* Seperates and reorders runs via fribidi using bidi algorithm*/
static int get_visual_runs(FriBidiCharType *types, FriBidiStrIndex length,
			   FriBidiParType par_type, FriBidiLevel *levels,
			   hb_script_t *scripts, Run *run)
{
    int max_level = levels[0];
    int i;
    for (i = 0; i < length; i++) {
	if(max_level < levels[i])
	    max_level = levels[i];
    }
    max_level++;

    int run_count = 0;
    FriBidiLevel lastLevel = -1;
    hb_script_t lastScript = -1;
    for (i = 0; i < length; i++) {
	int level = levels[i];
	int script = scripts[i];
	if (level != lastLevel || script != lastScript )
	    run_count += 1;
	lastLevel = level;
	lastScript = script;
    }

    if (run == NULL)
	return run_count;

    int start = 0;
    int index = 0;
    while (start < length) {
	int run_length = 0;
	while ((start + run_length) < length && levels[start] == levels[start + run_length]
					     && scripts[start] == scripts[start + run_length])
	    run_length++;
	run[index].start = start;
	run[index].level = levels[start];
	run[index].length = run_length;
	run[index].hb_script = scripts[start];
	start += run_length;
	index++;
    }

#ifdef TESTING
    TEST("\n");
    TEST("Run count: %d\n\n",run_count);
    TEST("Before reverse:\n");
    for (i = 0; i < run_count; ++i) {
        char buff[4];
        hb_tag_to_string(hb_script_to_iso15924_tag (run[i].hb_script),buff);
        TEST("run[%d]:\t start: %d\tlength: %d\tlevel: %d\tscript: %s\n",i,run[i].start,run[i].length,run[i].level,buff);
    }
#endif

    /* Implementation of L1 from unicode bidi algorithm */
    for (i = length - 1; (i >= 0) && FRIBIDI_IS_EXPLICIT_OR_BN_OR_WS (types[i]); i--)
	levels[i] = FRIBIDI_DIR_TO_LEVEL (par_type);

    /* Implementation of L2 from unicode bidi algorithm */
    int curr_level ;
    for (curr_level = max_level; curr_level > 0; curr_level--) {
	for (i = run_count-1 ; i >= 0 ; i-- ) {
	    if (run[i].level >= curr_level ) {
		int end = i;
		for (i-- ; (i >= 0 && run[i].level >= curr_level) ; i--)
		    ;
		reverse_run (run + i + 1 , end - i);
	    }
	}
    }

#ifdef TESTING
    TEST("\n");
    TEST("After reverse:\n");
    for (i = 0; i < run_count; ++i) {
	char buff[4];
	hb_tag_to_string(hb_script_to_iso15924_tag (run[i].hb_script),buff);
        TEST("run[%d]:\t start: %d\tlength: %d\tlevel: %d\tscript: %s\n",i,run[i].start,run[i].length,run[i].level,buff);
    }
    TEST("\n");
#endif

    free(levels);
    return run_count;
}

/* Does the shaping for each run buffer */
static void harfbuzz_shape(FriBidiChar *uni_str, FriBidiStrIndex length,
			   hb_font_t *hb_font, Run *run)
{
    run->hb_buffer = hb_buffer_create ();

    /* adding text to current buffer */
    hb_buffer_add_utf32(run->hb_buffer, uni_str, length, run->start, run->length);
    /* setting script of current buffer */
    hb_buffer_set_script(run->hb_buffer, run->hb_script);
    /* setting language of current buffer */
    hb_buffer_set_language (run->hb_buffer, hb_language_get_default());
    /* setting direction of current buffer */
    if (FRIBIDI_LEVEL_IS_RTL(run->level))
	hb_buffer_set_direction(run->hb_buffer, HB_DIRECTION_RTL);
    else
	hb_buffer_set_direction(run->hb_buffer, HB_DIRECTION_LTR);

    /* shaping current buffer */
    hb_shape(hb_font, run->hb_buffer, NULL, 0);
}


/* Takes the input text and does the reordering and shaping */
raqm_glyph_info_t *raqm_shape(const char *text , FT_Face face, raqm_direction_t direction) {
    int i = 0;
    const char *str = text;
    FriBidiStrIndex size = strlen(str);

    FriBidiChar *uni_str = (FriBidiChar*) malloc (sizeof (FriBidiChar) * size);
    FriBidiStrIndex length = fribidi_charset_to_unicode (FRIBIDI_CHAR_SET_UTF8, str, size, uni_str);

    FriBidiCharType *types = (FriBidiCharType*) malloc (sizeof (FriBidiCharType) * length);
    FriBidiLevel *levels = (FriBidiLevel*) malloc (sizeof (FriBidiLevel) * length);
    fribidi_get_bidi_types (uni_str, length, types);

    FriBidiParType par_type = FRIBIDI_PAR_ON;
    if (direction == RAQM_DIRECTION_RTL)
        par_type = FRIBIDI_PAR_RTL;
    else if (direction == RAQM_DIRECTION_LTR)
        par_type = FRIBIDI_PAR_LTR;

    TEST("Text is: %s\n\n", text);

    fribidi_get_par_embedding_levels (types, length, &par_type, levels);

    /* Handeling script detection for each character of the input string,
	if the character script is common or inherited it takes the script
	of the character before it except some special paired characters */
    hb_script_t *scripts = (hb_script_t*) malloc (sizeof (hb_script_t) * length);
    hb_unicode_funcs_t *ufuncs = hb_unicode_funcs_get_default();

#ifdef TESTING
    TEST("Before script detection:\n");
    for (i = 0; i < length; ++i) {
	scripts[i] = hb_unicode_script(ufuncs, uni_str[i]);
	char buff[4];
	hb_tag_to_string(hb_script_to_iso15924_tag (scripts[i]),buff);
	TEST("script for ch[%d]\t%s\n",i ,buff);
    }
#endif

    hb_script_t lastScriptValue;
    int lastScriptIndex = -1;
    int lastSetIndex = -1;
    Stack *scpt_stack = create(length);
    for (i = 0; i < length; ++i) {
	if (scripts[i] == HB_SCRIPT_COMMON && lastScriptIndex != -1) {
	    int pair_index = get_pair_index (uni_str[i]);
	    if (pair_index >= 0) {	/* is a paired character */
		if (IS_OPEN (pair_index)) {
		    scripts[i] = lastScriptValue;
		    lastSetIndex = i;
		    push(scpt_stack, scripts[i], pair_index);
		} else {	    /* is a close paired character */
		    int pi = pair_index & ~1; /* find matching opening (by getting the last even index for currnt odd index)*/
		    while (STACK_IS_NOT_EMPTY(scpt_stack) && scpt_stack->pi[scpt_stack->size] != pi)
			pop(scpt_stack);
		    if(STACK_IS_NOT_EMPTY(scpt_stack)) {
			scripts[i] = top(scpt_stack);
			lastScriptValue = scripts[i];
			lastSetIndex = i;
		    } else {
			scripts[i] = lastScriptValue;
			lastSetIndex = i;
		    }
		}
	    } else {
		scripts[i] = lastScriptValue;
		lastSetIndex = i;
	    }
	} else if (scripts[i] == HB_SCRIPT_INHERITED && lastScriptIndex != -1) {
	    scripts[i] = lastScriptValue;
	    lastSetIndex = i;
	} else {
	    int j;
	    for (j = lastSetIndex + 1; j < i; ++j)
		scripts[j] = scripts[i];
	    lastScriptValue = scripts[i];
	    lastScriptIndex = i;
	    lastSetIndex = i;
	}
    }

#ifdef TESTING
    TEST("\nAfter script detection:\n");
    for (i = 0; i < length; ++i) {
	char buff[4];
	hb_tag_to_string(hb_script_to_iso15924_tag (scripts[i]),buff);
	TEST("script for ch[%d]\t%s\n",i ,buff);
    }
#endif

    /* to get number of runs */
    int run_count = get_visual_runs(types, length, par_type, levels, scripts, NULL);
    Run *run = (Run*) malloc(sizeof(Run) * run_count);
    /* to populate run array */
    get_visual_runs(types, length, par_type, levels, scripts, run);

    /* harfbuzz shaping */
    hb_font_t *hb_font;
    hb_font = hb_ft_font_create (face, NULL);
    unsigned int glyph_count;
    int total_glyph_count = 0;
    unsigned int pos_length;
    hb_glyph_info_t *hb_glyph_info;
    hb_glyph_position_t *hb_glyph_position;

    for (i = 0; i < run_count; i++) {
	harfbuzz_shape(uni_str, length, hb_font, &run[i]);
	hb_glyph_info = hb_buffer_get_glyph_infos(run[i].hb_buffer, &glyph_count);
	total_glyph_count += glyph_count ;
    }
    raqm_glyph_info_t *g_info = (raqm_glyph_info_t*) malloc(sizeof(raqm_glyph_info_t) * total_glyph_count + 1);
    int index = 0;
    TEST("Glyph information:\n");
    for (i = 0; i < run_count; i++) {
	hb_glyph_info = hb_buffer_get_glyph_infos(run[i].hb_buffer, &glyph_count);
	hb_glyph_position = hb_buffer_get_glyph_positions (run[i].hb_buffer, &pos_length);
	int j;
	for (j = 0; j < glyph_count; j++){
	    g_info[index].index = hb_glyph_info[j].codepoint;
	    g_info[index].x_offset = hb_glyph_position[j].x_offset;
	    g_info[index].y_offset = hb_glyph_position[j].y_offset;
	    g_info[index].x_advanced = hb_glyph_position[j].x_advance;
	    TEST("glyph [%d]\tx_offset: %d\ty_offset: %d\tx_advance: %d\n",
	         g_info[index].index, g_info[index].x_offset,
	         g_info[index].y_offset, g_info[index].x_advanced);
	    index++;
	}
    }
    g_info[total_glyph_count].index = -1;

    free(types);
    free(uni_str);
    free(run);

    return g_info;
}

