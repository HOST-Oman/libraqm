/*
 * Copyright © 2015 Information Technology Authority (ITA) <foss@ita.gov.om>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef HAVE_FRIBIDI_REORDER_RUNS
#include <assert.h>
#include <string.h>

#include "reorder_runs.h"

static void reverse_run (FriBidiRun *arr, const FriBidiStrIndex len)
{
  FriBidiStrIndex i;

  assert (arr);

  for (i = 0; i < len / 2; i++)
    {
      FriBidiRun temp = arr[i];
      arr[i] = arr[len - 1 - i];
      arr[len - 1 - i] = temp;
    }
}

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

  if (len == 0)
    {
      goto out;
    }

  assert (bidi_types);
  assert (embedding_levels);

  /* L1. Reset the embedding levels of some chars:
     4. any sequence of white space characters at the end of the line. */
  for (i = len - 1; i >= 0 &&
       FRIBIDI_IS_EXPLICIT_OR_BN_OR_WS (bidi_types[i]); i--)
     embedding_levels[i] = FRIBIDI_DIR_TO_LEVEL (base_dir);

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
#endif /* HAVE_FRIBIDI_REORDER_RUNS */
