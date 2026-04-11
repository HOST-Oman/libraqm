/*
 * Unicode Grapheme Cluster Break conformance test.
 *
 * Parses GraphemeBreakTest.txt and verifies that
 * _raqm_allowed_grapheme_boundary produces correct results.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern bool
_raqm_allowed_grapheme_boundary (const uint32_t *text,
                                 size_t          len,
                                 size_t          index);

static int
run_tests (const char *path)
{
  FILE *f = fopen (path, "r");
  if (!f)
  {
    fprintf (stderr, "Cannot open %s\n", path);
    return 1;
  }

  char line[4096];
  int test_num = 0;
  int failures = 0;

  while (fgets (line, sizeof (line), f))
  {
    /* Skip comments and empty lines */
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
      continue;

    /* Strip trailing comment */
    char *comment = strchr (line, '#');
    if (comment)
      *comment = '\0';

    /* Parse: ÷ and × are multi-byte UTF-8 */
    uint32_t codepoints[256];
    bool breaks[256]; /* breaks[i] = boundary before codepoints[i] */
    size_t n = 0;
    bool expect_break = false;

    const char *p = line;
    while (*p)
    {
      /* Skip whitespace */
      while (*p == ' ' || *p == '\t')
        p++;

      if (!*p || *p == '\n' || *p == '\r')
        break;

      /* Check for ÷ (UTF-8: C3 B7) or × (UTF-8: C3 97) */
      if ((unsigned char) p[0] == 0xC3)
      {
        if ((unsigned char) p[1] == 0xB7)
        {
          expect_break = true;
          p += 2;
          continue;
        }
        else if ((unsigned char) p[1] == 0x97)
        {
          expect_break = false;
          p += 2;
          continue;
        }
      }

      /* Parse hex codepoint */
      char *end;
      unsigned long cp = strtoul (p, &end, 16);
      if (end == p)
      {
        p++;
        continue;
      }
      p = end;

      breaks[n] = expect_break;
      codepoints[n] = (uint32_t) cp;
      n++;
    }

    if (n == 0)
      continue;

    test_num++;

    /* Test each boundary position.
     * breaks[i] for i > 0 tells us whether there should be a break
     * between codepoints[i-1] and codepoints[i], which corresponds to
     * _raqm_allowed_grapheme_boundary(codepoints, n, i-1). */
    for (size_t i = 1; i < n; i++)
    {
      bool expected = breaks[i];
      bool actual = _raqm_allowed_grapheme_boundary (codepoints, n, i - 1);
      if (actual != expected)
      {
        fprintf (stderr,
                 "FAIL test %d: between U+%04X and U+%04X (pos %zu): "
                 "expected %s, got %s\n",
                 test_num, codepoints[i - 1], codepoints[i], i - 1,
                 expected ? "÷ (break)" : "× (no break)",
                 actual ? "÷ (break)" : "× (no break)");
        failures++;
      }
    }
  }

  fclose (f);

  if (failures)
    fprintf (stderr, "%d failures in %d tests\n", failures, test_num);
  else
    printf ("All %d tests passed.\n", test_num);

  return failures ? 1 : 0;
}

int
main (int argc, char **argv)
{
  if (argc != 2)
  {
    fprintf (stderr, "Usage: %s <GraphemeBreakTest.txt>\n", argv[0]);
    return 1;
  }

  return run_tests (argv[1]);
}
