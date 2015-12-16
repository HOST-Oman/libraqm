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

#ifndef _SHAPE_TEXT_H_
#define _SHAPE_TEXT_H_

/* For enabling debug mode */
/*#define DEBUG 1*/
#ifdef DEBUG
#define DBG(...) fprintf (stderr, __VA_ARGS__)
#else
#define DBG(...)
#endif

#ifdef TESTING
#define TEST(...) printf (__VA_ARGS__)
#else
#define TEST(...)
#endif

#include <ft2build.h>
#include <fribidi.h>
#include <hb.h>
#include <hb-ft.h>
#include <assert.h>

/* Final glyph information gained from harfbuzz */
typedef struct
{
    int index;         /* Glyph index */
    int x_offset;      /* Horizontal glyph offset */
    int x_advance;     /* Horizontal glyph advance width */
    int y_offset;      /* Vertical glyph offset */
    uint32_t cluster;  /* Index of original character in input text (in UTF-32 code points) */
} raqm_glyph_info_t;

typedef enum
{
    RAQM_DIRECTION_DEFAULT,  /* Automatic paragraph direction based on first strong character */
    RAQM_DIRECTION_RTL,      /* Right-To-Left paragraph */
    RAQM_DIRECTION_LTR       /* Left-To-Right paragraph */
} raqm_direction_t;

/* raqm_shape - apply bidi algorithm and shape text.
 *
 * This function reorders and shapes the text using FriBiDi and HarfBuzz.
 * It supports proper script detection for each character of the input string.
 * If the character script is common or inherited it takes the script of the
 * character before it except some special paired characters.
 *
 * You should provide the input text, FreeType face and paragraph base direction.
 *
 * Returns: array of raqm_glyph_info_t.
 */
raqm_glyph_info_t* raqm_shape (const char* text, FT_Face face, raqm_direction_t direction);

#endif
