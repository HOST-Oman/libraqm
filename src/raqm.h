/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
 * Copyright © 2016 Khaled Hosny <khaledhosny@eglug.org>
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

#ifndef _RAQM_H_
#define _RAQM_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef __cplusplus
extern "C" {
#endif

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

/**
 * raqm_alignment_t:
 * @RAQM_ALIGNMENT_START: Same as #RAQM_ALIGNMENT_RIGHT if paragraph direction
 *                        is #RAQM_DIRECTION_RTL, same as #RAQM_ALIGNMENT_LEFT
 *                        otherwise.
 * @RAQM_ALIGNMENT_END: Same as #RAQM_ALIGNMENT_LEFT if paragraph direction is
 *                      #RAQM_DIRECTION_RTL, same as #RAQM_ALIGNMENT_RIGHT
 *                      otherwise.
 * @RAQM_ALIGNMENT_RIGHT: Paragraph is right aligned.
 * @RAQM_ALIGNMENT_LEFT: Paragraph is left aligned.
 * @RAQM_ALIGNMENT_CENTER: Paragraph is center aligned..
 * @RAQM_ALIGNMENT_JUSTIFY: Paragraph is justified.
 *
 * Base paragraph alignment, see raqm_set_alignment().
 *
 * Since: 0.3
 */
typedef enum
{
    RAQM_ALIGNMENT_START,
    RAQM_ALIGNMENT_END,
    RAQM_ALIGNMENT_RIGHT,
    RAQM_ALIGNMENT_LEFT,
    RAQM_ALIGNMENT_CENTER,
    RAQM_ALIGNMENT_JUSTIFY,
} raqm_alignment_t;

/**
 * raqm_glyph_t:
 * @index: the index of the glyph in the font file.
 * @x_advance: the glyph advance width in horizontal text.
 * @y_advance: the glyph advance width in vertical text.
 * @x_offset: the horizontal movement of the glyph from the current point.
 * @y_offset: the vertical movement of the glyph from the current point.
 * @x: the absolute x position of the glyph.
 * @y: the absolute y position of the glyph.
 * @cluster: the index of original character in input text.
 * @ftface: the @FT_Face of the glyph.
 *
 * The structure that holds information about output glyphs, returned from
 * raqm_get_glyphs().
 */
typedef struct raqm_glyph_t {
    unsigned int index;
    int x_advance;
    int y_advance;
    int x_offset;
    int y_offset;
    uint32_t cluster;
    FT_Face ftface;
    int x;
    int y;
    /*< private >*/
    int visual_index;
    int line;
} raqm_glyph_t;

raqm_t *
raqm_create (void);

raqm_t *
raqm_reference (raqm_t *rq);

void
raqm_destroy (raqm_t *rq);

bool
raqm_set_text (raqm_t         *rq,
               const uint32_t *text,
               size_t          len);

bool
raqm_set_text_utf8 (raqm_t     *rq,
                    const char *text,
                    size_t      len);

bool
raqm_set_par_direction (raqm_t          *rq,
                        raqm_direction_t dir);

bool
raqm_set_language (raqm_t       *rq,
                   const char   *lang,
                   size_t        start,
                   size_t        len);

bool
raqm_set_width (raqm_t *rq,
                int    width);

bool
raqm_set_alignment (raqm_t           *rq,
                    raqm_alignment_t alignment);

bool
raqm_add_font_feature  (raqm_t     *rq,
                        const char *feature,
                        int         len);

bool
raqm_set_freetype_face (raqm_t *rq,
                        FT_Face face);

bool
raqm_set_freetype_face_range (raqm_t *rq,
                              FT_Face face,
                              size_t  start,
                              size_t  len);

bool
raqm_set_freetype_load_flags (raqm_t *rq,
                              int flags);

bool
raqm_layout (raqm_t *rq);

raqm_glyph_t *
raqm_get_glyphs (raqm_t *rq,
                 size_t *length);

bool
raqm_index_to_position (raqm_t *rq,
                        size_t *index,
                        int *x,
                        int *y);

bool
raqm_position_to_index (raqm_t *rq,
                        int x,
                        int y,
                        size_t *index);

#ifdef __cplusplus
}
#endif
#endif /* _RAQM_H_ */
