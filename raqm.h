#ifndef _SHAPE_TEXT_H_
#define _SHAPE_TEXT_H_

#include <ft2build.h>
#include <fribidi.h>
#include <hb.h>
#include <hb-ft.h>

/* Final glyph information gained from harfbuzz */
typedef struct _glyph_info {
    int index;
    int x_offset;
    int x_advanced;
    int y_offset;
} glyph_info;

glyph_info *shape_text (const char *text , FT_Face face);

#endif
