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

#ifdef _MSC_VER
#include<malloc.h>
#endif

#include <assert.h>
#include <locale.h>
#include <stdio.h>
#include "raqm.h"

int getcount(char *str, char *delim)
{
    int i, n = 0;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == delim[0])
            n++;
    }
    return n;
}

char** strsplit(char *str, char *delim, int count)
{
    int n = 0;
    char **list;

    list = (char**)malloc(sizeof(char*) * count);
    char *result = NULL;
    result = strtok(str, delim);
    n = 0;
    while( result != NULL ) {
        list[n] = (char*)malloc(sizeof(char*) * (strlen(result) + 1));
        strcpy(list[n], result);
        n ++;
        result = strtok(NULL, delim);
    }
    return list;
}

void free_list(char** array, int count)
{
    for(int i = 0; i < count; ++i)
        free(array[i]);
#ifdef _MSC_VER
    _freea(array);
#else
    free(array);
#endif
    
}

int removeescape(char *str)
{
    static const char escape[256] = {
        ['a'] = '\a',
        ['b'] = '\b',
        ['f'] = '\f',
        ['n'] = '\n',
        ['r'] = '\r',
        ['t'] = '\t',
        ['v'] = '\v',
        ['\\'] = '\\',
        ['\''] = '\'',
        ['"'] = '\"',
        ['?'] = '\?',
    };

    char *p = str;      /* Pointer to original string */
    char *q = str;      /* Pointer to new string; q <= p */

    while (*p) {
        int c = *(unsigned char*)p++;

        if (c == '\\') {
            c = *(unsigned char*)p++;
            if (c == '\0') break;
            if (escape[c]) c = escape[c];
        }

        *q++ = c;
    }
    *q = '\0';

    return q - str;
}
int
main (int argc, char *argv[])
{
    const char *testname = NULL;
    char *direction = NULL;
    char *text = NULL;
    char *features = NULL;
    char *font = NULL;
    char *fonts = NULL;
    char *languages = NULL;
    int cluster = -1;
    int position = -1;
    int i;
    testname = argv[1];
    int size = 0;
    FILE *testFile;
    testFile = fopen(testname, "r");

    char buf[255];     /* Buffer for a line of input. */
    char* VarName = NULL;
    char* VarValue = NULL;

    while(feof(testFile)== 0){
        fgets(buf, 255, testFile);
        strtok(buf, "\n\r");
        char delims[] = "=";
        VarName = strtok( buf, delims );

        VarValue = strtok( NULL, delims );

        if (strcmp(VarName, "font") == 0)
        {
            font = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (font , VarValue);
        }
        else if (strcmp(VarName, "text") == 0)
        {
            text = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (text , VarValue);
            removeescape(text);
        }
        else if (strcmp(VarName, "direction") == 0)
        {
            direction = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (direction , VarValue);
        }
        else if (strcmp(VarName, "features") == 0)
        {
            features = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (features , VarValue);
        }
        else if (strcmp(VarName, "fonts") == 0)
        {
            fonts = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (fonts , VarValue);
        }
        else if (strcmp(VarName, "languages") == 0)
        {
            languages = malloc(sizeof(char*) * strlen(VarValue));
            strcpy (languages , VarValue);
        }
        else if (strcmp(VarName, "cluster") == 0)
        {
            cluster = atoi(VarValue);
        }
        else if (strcmp(VarName, "position") == 0)
        {
            position = atoi(VarValue);
        }

    }

    fclose(testFile);

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
        size = getcount(fonts, ",");
#ifdef _MSC_VER
        char** list = _malloca(size * sizeof(char*));
#else
        char** list;
#endif
        list = strsplit(fonts, ",", size);
        assert (!FT_New_Face (library, list[0], 0, &face));
        assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
        assert (raqm_set_freetype_face_range(rq, face, atoi(list[1]), atoi(list[2])));

        assert (!FT_New_Face (library, list[3], 0, &face));
        assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
        assert (raqm_set_freetype_face_range(rq, face, atoi(list[4]), atoi(list[5])));

        free_list(list, size);

    } else {
        assert (!FT_New_Face (library, font, 0, &face));
        assert (!FT_Set_Char_Size (face, face->units_per_EM, 0, 0, 0));
        assert (raqm_set_freetype_face (rq, face));
    }

    if (languages)
    {
        size = getcount(languages, ":");
#ifdef _MSC_VER
        char** list = _malloca(size * sizeof(char*));
#else
        char** list;
#endif
        list = strsplit(languages, ":", size);
        for (i = 0; i < size; i++)
        {
            int num = getcount(list[i], ",");
#ifdef _MSC_VER
            char** sublist = _malloca(num * sizeof(char*));
#else
            char** sublist;
#endif
            sublist = strsplit(list[i], ",", num);
            assert (raqm_set_language(rq, sublist[0], atoi(sublist[1]), atoi(sublist[2])));
            free_list(sublist, num);
        }

        free_list(list, size);
    }

    if (features)
    {
        size = getcount(features, ",");
#ifdef _MSC_VER
        char** list = _malloca(size * sizeof(char*));
#else
        char** list;
#endif
        list = strsplit(features, ",", size);
        for (i = 0; i < size; i++)
        {
            assert (raqm_add_font_feature (rq, list[i], -1));
        }
        free_list(list, size);
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

    if (font)
        free(font);
    if (text)
        free(text);
    if (direction)
        free(direction);
    if (languages)
        free(languages);
    if (fonts)
        free(fonts);
    if (features)
        free(features);

    return 0;
}
