/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
 * Copyright © 2016 Khaled Hosny <khaledhosny@eglug.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <locale.h>
#include "raqm.h"


int
main (int argc, char *argv[])
{
    const char *testname = NULL;
    const char *direction = NULL;
    const char *text = NULL;
    const char *features = NULL;
    const char *font = NULL;
    const char *fonts = NULL;
    const char *languages = NULL;
    const char *tmpFont = NULL;
    int cluster = -1;
    int position = -1;

    testname = argv[1];

    if (strcmp(testname, "test1.test") == 0){
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "عربي(English ) عربي";
    }
    else if (strcmp(testname, "test1_LTR.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "عربي(English ) عربي";
        direction = "ltr";
    }
    else if (strcmp(testname, "test1_RTL.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "عربي(English ) عربي";
        direction = "rtl";
    }
    else if (strcmp(testname, "test2.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123";
    }
    else if (strcmp(testname, "test2_LTR.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123";
        direction = "ltr";
    }
    else if (strcmp(testname, "test2_RTL.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123";
        direction = "rtl";
    }
    else if (strcmp(testname, "test3.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123 عمان english";
    }
    else if (strcmp(testname, "test3_LTR.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123 عمان english";
        direction = "ltr";
    }
    else if (strcmp(testname, "test3_RTL.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "arabic عربي 123 عمان english";
        direction = "rtl";
    }
    else if (strcmp(testname, "test4.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "بيت سالم مصلى عمان";
    }
    else if (strcmp(testname, "test4_LTR.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "بيت سالم مصلى عمان";
        direction = "ltr";
    }
    else if (strcmp(testname, "test4_RTL.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "بيت سالم مصلى عمان";
        direction = "rtl";
    }
    else if (strcmp(testname, "test5.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "aa (bb) aa";
    }
    else if (strcmp(testname, "test5_LTR.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "aa (bb) aa";
        direction = "ltr";
    }
    else if (strcmp(testname, "test5_RTL.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "aa (bb) aa";
        direction = "rtl";
    }
    else if (strcmp(testname, "cursor_position1.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "ömán عُمان";
        cluster = 1;
        position = 1000;
    }
    else if (strcmp(testname, "cursor_position2.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "ömán عُمان";
        cluster = 10;
        position = 6000;
    }
    else if (strcmp(testname, "cursor_position3.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "عمَان oman";
        cluster = 0;
        position = 7000;
    }
    else if (strcmp(testname, "cursor_position4.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "عمَان oman";
        cluster = 4;
        position = 5000;
    }
    else if (strcmp(testname, "cursor_position_GB3.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "om\r\nan";
        cluster = 2;
        position = 3000;
    }
    else if (strcmp(testname, "cursor_position_GB4.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "omanoman";
        cluster = 3;
        position = 4000;
    }
    else if (strcmp(testname, "cursor_position_GB5.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "omanoman";
        cluster = 4;
        position = 5000;
    }
    else if (strcmp(testname, "cursor_position_GB8a.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "\xf0\x9f\x87\xa6\xf0\x9f\x87\xa7\xf0\x9f\x87\xa8\xf0\x9f\x87\xa9";
        cluster = 1;
        position = 1000;
    }
    else if (strcmp(testname, "cursor_position_GB9.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "ömán عُمان";
        cluster = 1;
        position = 1000;
    }
    else if (strcmp(testname, "cursor_position_GB9a.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "अीअ";
        cluster = 0;
        position = 300;
    }
    else if (strcmp(testname, "scripts-backward.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "عربيעבריתأهلבריתمم(ُenglish ) مرحبا";
        cluster = 0;
        position = 300;
    }
    else if (strcmp(testname, "scripts-backward-ltr.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "عربيעבריתأهلבריתمم(ُenglish ) مرحبا";
        direction = "ltr";
    }
    else if (strcmp(testname, "scripts-backward-rtl.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "عربيעבריתأهلבריתمم(ُenglish ) مرحبا";
        direction = "rtl";
    }
    else if (strcmp(testname, "scripts-forward.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "abcd (αβγ) δабгд";
        cluster = 0;
        position = 300;
    }
    else if (strcmp(testname, "scripts-forward-ltr.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "abcd (αβγ) δабгд";
        direction = "ltr";
    }
    else if (strcmp(testname, "scripts-forward-rtl.test") == 0)
    {
        font = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
        text = "abcd (αβγ) δабгд";
        direction = "rtl";
    }
    else if (strcmp(testname, "features-arabic.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "اللغة العربية";
        features = "-fina,-init,-medi";
    }
    else if (strcmp(testname, "features-kerning.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "Linux Support";
        features = "-kern";
    }
    else if (strcmp(testname, "features-ligature.test") == 0)
    {
        font = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
        text = "file is filling";
        features = "-liga";
    }
    else if (strcmp(testname, "languages-sr.test") == 0)
    {
        font = "fonts/db17d357d9d813a863c3859d2f3af11faa0b39c7.ttf";
        text = "тб";
        direction = "ltr";
        languages = "sr;0:4";
    }
    else if (strcmp(testname, "languages-sr-ru.test") == 0)
    {
        font = "fonts/db17d357d9d813a863c3859d2f3af11faa0b39c7.ttf";
        text = "тбтб";
        direction = "ltr";
        languages = "sr;0:4,ru;4:4";
    }
    else if (strcmp(testname, "xyoffset.test") == 0)
    {
        font = "fonts/a22e097e7f3cefffd1a602674dff5108efa0eec2.ttf";
        text = "حجج";
    }
    else if (strcmp(testname, "multi-fonts.test") == 0)
    {
        fonts = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf 0:12,fonts/sha1sum/a22e097e7f3cefffd1a602674dff5108efa0eec2.ttf 12:21";
        text = "English اللغة العربية";
    }
    else if (strcmp(testname, "multi-fonts2.test") == 0)
    {
        fonts = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf 0:8,fonts/sha1sum/d46a2549d27c32605024201abf801bb9a9273da3.ttf 2:4";
        text = "عربي";
    }
    else
    {
        fprintf(stderr, "Test Name is missing.\n\n");
        fprintf(stderr, "usage: raqm-test testname\n");
        return 1;
    }

    FT_Library library;
    FT_Face face;

    raqm_t *rq;
    raqm_glyph_t *glyphs;
    size_t count, start_index, index;
    raqm_direction_t dir;
    int x = 0, y = 0;

    setlocale (LC_ALL, "");

    char *error = NULL;

    if (text == NULL || (font == NULL && fonts == NULL))
    {
        if (text == NULL)
            error = "Text";
        else
            error = "font[s]";
        fprintf(stderr, "%s is missing.\n\n", error);
        return 1;
    }
    dir = RAQM_DIRECTION_DEFAULT;
    if (direction && strcmp(direction, "rtl") == 0)
        dir = RAQM_DIRECTION_RTL;
    else if (direction && strcmp(direction, "ltr") == 0)
        dir = RAQM_DIRECTION_LTR;

    rq = raqm_create ();
    assert (raqm_set_text_utf8 (rq, text, strlen (text)));
    assert (raqm_set_par_direction (rq, dir));
    assert (!FT_Init_FreeType (&library));

    if (fonts)
    {
        if (strcmp(testname, "multi-fonts.test") == 0)
        {
            tmpFont = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
            assert (!FT_New_Face (library, tmpFont, 0, &face));
            assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
            assert (raqm_set_freetype_face_range(rq, face, 0, 12));

            tmpFont = "fonts/a22e097e7f3cefffd1a602674dff5108efa0eec2.ttf";
            assert (!FT_New_Face (library, tmpFont, 0, &face));
            assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
            assert (raqm_set_freetype_face_range(rq, face, 12, 21));

        }
        else if (strcmp(testname, "multi-fonts2.test") == 0)
        {
            tmpFont = "fonts/bcb3b98eb67ece19b8b709f77143d91bcb3d95eb.ttf";
            assert (!FT_New_Face (library, tmpFont, 0, &face));
            assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
            assert (raqm_set_freetype_face_range(rq, face, 0, 8));

            tmpFont = "fonts/d46a2549d27c32605024201abf801bb9a9273da3.ttf";
            assert (!FT_New_Face (library, tmpFont, 0, &face));
            assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
            assert (raqm_set_freetype_face_range(rq, face, 2, 4));
        }

    } else {
        assert (!FT_New_Face (library, font, 0, &face));
        assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
        assert (raqm_set_freetype_face (rq, face));
    }

    if (languages)
    {
        if (strcmp(testname, "languages-sr.test") == 0)
        {
            assert (raqm_set_language(rq, "sr", 0, 4));
        }
        else if (strcmp(testname, "languages-sr-ru.test") == 0)
        {
            assert (raqm_set_language(rq, "sr", 0, 4));
            assert (raqm_set_language(rq, "ru", 4, 4));
        }
    }

    if (features)
    {
        if (strcmp(testname, "features-arabic.test") == 0)
        {
            assert (raqm_add_font_feature (rq, "-fina", -1));
            assert (raqm_add_font_feature (rq, "-init", -1));
            assert (raqm_add_font_feature (rq, "-medi", -1));
        }
        else if (strcmp(testname, "features-kerning.test") == 0)
        {
            assert (raqm_add_font_feature (rq, "-kern", -1));
        }
        else if (strcmp(testname, "features-ligature.test") == 0)
        {
            assert (raqm_add_font_feature (rq, "-liga", -1));
        }
    }
    assert (raqm_layout (rq));

    glyphs = raqm_get_glyphs (rq, &count);
    assert (glyphs != NULL);

    if (cluster >= 0)
    {
        index = cluster;
        assert (raqm_index_to_position (rq, &index, &x, &y));
    }

    if (position)
        assert (raqm_position_to_index (rq, position, 0, &start_index));

    raqm_destroy (rq);
    FT_Done_Face (face);
    FT_Done_FreeType (library);

    return 0;
}
