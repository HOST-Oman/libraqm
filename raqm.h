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

/* Final glyph information gained from harfbuzz */
typedef struct {
    int index;
    int x_offset;
    int x_advanced;
    int y_offset;
} raqm_glyph_info_t;

typedef enum {
    RAQM_DIRECTION_DEFAULT,
    RAQM_DIRECTION_RTL,
    RAQM_DIRECTION_LTR
} raqm_direction_t;

raqm_glyph_info_t *raqm_shape(const char *text , FT_Face face, raqm_direction_t);

#endif
