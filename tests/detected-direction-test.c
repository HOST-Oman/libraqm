/*
 * Detected paragraph direction test.
 *
 * Verifies that raqm_get_par_detected_direction reports the natural direction
 * of the text as detected from its content, regardless of the base direction
 * set by the user, and reports direction-neutral text as
 * RAQM_DIRECTION_DEFAULT.
 */

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "raqm.h"

static raqm_direction_t
detect (const char *utf8, raqm_direction_t base)
{
  raqm_direction_t dir;
  raqm_t *rq = raqm_create ();
  assert (rq);
  assert (raqm_set_text_utf8 (rq, utf8, strlen (utf8)));
  assert (raqm_set_par_direction (rq, base));
  dir = raqm_get_par_detected_direction (rq);
  raqm_destroy (rq);
  return dir;
}

int
main (void)
{
  const char *ltr = "English";
  const char *rtl = "عربي";
  const char *neutral = "123.!";

  /* Detected direction follows the content. */
  assert (detect (ltr, RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_LTR);
  assert (detect (rtl, RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_RTL);
  assert (detect (neutral, RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_DEFAULT);

  /* ... regardless of the base direction set by the user. */
  assert (detect (ltr, RAQM_DIRECTION_RTL) == RAQM_DIRECTION_LTR);
  assert (detect (rtl, RAQM_DIRECTION_LTR) == RAQM_DIRECTION_RTL);
  assert (detect (neutral, RAQM_DIRECTION_RTL) == RAQM_DIRECTION_DEFAULT);
  assert (detect (neutral, RAQM_DIRECTION_LTR) == RAQM_DIRECTION_DEFAULT);

  /* It is decided by the first strong character (Unicode Bidi rule P2). */
  assert (detect ("A عربي", RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_LTR);
  assert (detect ("عربي A", RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_RTL);

  /* Leading weak/neutral characters are skipped to the first strong one. */
  assert (detect ("123 English", RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_LTR);
  assert (detect ("123 عربي", RAQM_DIRECTION_DEFAULT) == RAQM_DIRECTION_RTL);

  /* A vertical base direction is a layout property, not a content one, so it
   * is ignored and never reported back. */
  assert (detect (ltr, RAQM_DIRECTION_TTB) == RAQM_DIRECTION_LTR);
  assert (detect (rtl, RAQM_DIRECTION_TTB) == RAQM_DIRECTION_RTL);
  assert (detect (neutral, RAQM_DIRECTION_TTB) == RAQM_DIRECTION_DEFAULT);

  /* The direction is read from the text content whichever setter was used. */
  {
    /* UTF-16: Arabic ARE (U+0639 U+0631 U+0628 U+064A), all in the BMP. */
    const uint16_t utf16_rtl[] = { 0x0639, 0x0631, 0x0628, 0x064A };
    /* UTF-32: an astral strong-LTR character (U+1D400, outside the BMP). */
    const uint32_t utf32_ltr[] = { 0x1D400 };
    raqm_t *rq;

    rq = raqm_create ();
    assert (rq);
    assert (raqm_set_text_utf16 (rq, utf16_rtl, 4));
    assert (raqm_get_par_detected_direction (rq) == RAQM_DIRECTION_RTL);
    raqm_destroy (rq);

    rq = raqm_create ();
    assert (rq);
    assert (raqm_set_text (rq, utf32_ltr, 1));
    assert (raqm_get_par_detected_direction (rq) == RAQM_DIRECTION_LTR);
    raqm_destroy (rq);
  }

  /* Text that was never set, explicitly empty text, or a NULL handle is all
   * direction neutral. */
  {
    raqm_t *rq = raqm_create ();
    assert (rq);
    assert (raqm_get_par_detected_direction (rq) == RAQM_DIRECTION_DEFAULT);
    assert (raqm_set_text_utf8 (rq, "", 0));
    assert (raqm_get_par_detected_direction (rq) == RAQM_DIRECTION_DEFAULT);
    raqm_destroy (rq);
  }
  assert (raqm_get_par_detected_direction (NULL) == RAQM_DIRECTION_DEFAULT);

  return 0;
}
