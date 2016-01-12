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

#ifndef _RAQM_H_
#define _RAQM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <ft2build.h>
#include FT_FREETYPE_H

/**
 * raqm_t:
 *
 * This is the main object holding all state of the currently processed text as
 * well as its output.
 *
 * Since: 0.1
 */
typedef struct _raqm raqm_t;

/**
 * raqm_direction_t:
 * @RAQM_DIRECTION_DEFAULT: Detect paragraph direction automatically.
 * @RAQM_DIRECTION_RTL: Paragraph is mainly right-to-left text.
 * @RAQM_DIRECTION_LTR: Paragraph is mainly left-to-right text.
 * @RAQM_DIRECTION_TTB: Paragraph is mainly vertical top-to-bottom text.
 *
 * Base paragraph direction, see raqm_set_par_direction().
 *
 * Since: 0.1
 */
typedef enum
{
    RAQM_DIRECTION_DEFAULT,
    RAQM_DIRECTION_RTL,
    RAQM_DIRECTION_LTR,
    RAQM_DIRECTION_TTB
} raqm_direction_t;

raqm_t *
raqm_create (void);

raqm_t *
raqm_reference (raqm_t *rq);

void
raqm_destroy (raqm_t *rq);

void
raqm_add_text (raqm_t         *rq,
               const uint32_t *text,
               size_t          len);

void
raqm_set_par_direction (raqm_t          *rq,
                        raqm_direction_t dir);

bool
raqm_add_font_feature  (raqm_t     *rq,
                        const char *feature,
                        int         len);

void
raqm_set_freetype_face (raqm_t *rq,
                        FT_Face face,
                        size_t  start,
                        size_t  len);

bool
raqm_layout (raqm_t *rq);

/* Old API */

/* Output glyph */
typedef struct
{
    unsigned int index; /* Glyph index */
    int x_offset;       /* Horizontal glyph offset */
    int x_advance;      /* Horizontal glyph advance width */
    int y_advance;      /* Vertical glyph advance width */
    int y_offset;       /* Vertical glyph offset */
    uint32_t cluster;   /* Index of original character in input text */
} raqm_glyph_info_t;

/* raqm_shape - apply bidi algorithm and shape text.
 *
 * This function reorders and shapes the text using FriBiDi and HarfBuzz.
 * It supports proper script detection for each character of the input string.
 * If the character script is common or inherited it takes the script of the
 * character before it except some special paired characters.
 *
 * Returns: number of glyphs.
 */
unsigned int
raqm_shape     (const char* text,               /* input text, UTF-8 encoded */
                int length,                     /* length of text array  */
                const FT_Face face,             /* font to use for shaping */
                raqm_direction_t direction,     /* base paragraph direction */
                const char **features,          /* a NULL-terminated array of font feature strings */
                raqm_glyph_info_t** glyph_info  /* output glyph info, should be freed by the client */
               );

unsigned int
raqm_shape_u32 (const uint32_t* text,           /* input text, UTF-32 encoded */
                int length,                     /* length of text array */
                const FT_Face face,             /* font to use for shaping */
                raqm_direction_t direction,     /* base paragraph direction */
                const char **features,          /* a NULL-terminated array of font feature strings */
                raqm_glyph_info_t** glyph_info  /* output glyph info, should be freed by the client */
               );

#endif /* _RAQM_H_ */
