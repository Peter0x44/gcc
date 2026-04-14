/* identifier_to_locale support for pretty-printing diagnostics.
   Copyright (C) 2003-2026 Free Software Foundation, Inc.
   Contributed by Gabriel Dos Reis <gdr@integrable-solutions.net>

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#define INCLUDE_VECTOR
#include "system.h"
#include "coretypes.h"
#include "intl.h"
#include "pretty-print.h"
#include "pretty-print-utf8.h"

#if HAVE_ICONV
#include <iconv.h>
#endif

/* Allocator for identifier_to_locale and corresponding function to
   free memory.  */

void *(*identifier_to_locale_alloc) (size_t) = xmalloc;
void (*identifier_to_locale_free) (void *) = free;

/* Given IDENT, an identifier in the internal encoding, return a
   version of IDENT suitable for diagnostics in the locale character
   set: either IDENT itself, or a string, allocated using
   identifier_to_locale_alloc, converted to the locale character set
   and using escape sequences if not representable in the locale
   character set or containing control characters or invalid byte
   sequences.  Existing backslashes in IDENT are not doubled, so the
   result may not uniquely specify the contents of an arbitrary byte
   sequence identifier.  */

const char *
identifier_to_locale (const char *ident)
{
  const unsigned char *uid = (const unsigned char *) ident;
  size_t idlen = strlen (ident);
  bool valid_printable_utf8 = true;
  bool all_ascii = true;
  size_t i;

  for (i = 0; i < idlen;)
    {
      unsigned int c;
      size_t utf8_len = pp_decode_utf8_char (&uid[i], idlen - i, &c);
      if (utf8_len == 0 || c <= 0x1F || (c >= 0x7F && c <= 0x9F))
	{
	  valid_printable_utf8 = false;
	  break;
	}
      if (utf8_len > 1)
	all_ascii = false;
      i += utf8_len;
    }

  /* If IDENT contains invalid UTF-8 sequences (which may occur with
     attributes putting arbitrary byte sequences in identifiers), or
     control characters, we use octal escape sequences for all bytes
     outside printable ASCII.  */
  if (!valid_printable_utf8)
    {
      char *ret = (char *) identifier_to_locale_alloc (4 * idlen + 1);
      char *p = ret;
      for (i = 0; i < idlen; i++)
	{
	  if (uid[i] > 0x1F && uid[i] < 0x7F)
	    *p++ = uid[i];
	  else
	    {
	      sprintf (p, "\\%03o", uid[i]);
	      p += 4;
	    }
	}
      *p = 0;
      return ret;
    }

  /* Otherwise, if it is valid printable ASCII, or printable UTF-8
     with the locale character set being UTF-8, IDENT is used.  */
  if (all_ascii || locale_utf8)
    return ident;

  /* Otherwise IDENT is converted to the locale character set if
     possible.  */
#if defined ENABLE_NLS && defined HAVE_LANGINFO_CODESET && HAVE_ICONV
  if (locale_encoding != NULL)
    {
      iconv_t cd = iconv_open (locale_encoding, "UTF-8");
      bool conversion_ok = true;
      char *ret = NULL;
      if (cd != (iconv_t) -1)
	{
	  size_t ret_alloc = 4 * idlen + 1;
	  for (;;)
	    {
	      /* Repeat the whole conversion process as needed with
		 larger buffers so non-reversible transformations can
		 always be detected.  */
	      ICONV_CONST char *inbuf = const_cast<char *> (ident);
	      char *outbuf;
	      size_t inbytesleft = idlen;
	      size_t outbytesleft = ret_alloc - 1;
	      size_t iconv_ret;

	      ret = (char *) identifier_to_locale_alloc (ret_alloc);
	      outbuf = ret;

	      if (iconv (cd, 0, 0, 0, 0) == (size_t) -1)
		{
		  conversion_ok = false;
		  break;
		}

	      iconv_ret = iconv (cd, &inbuf, &inbytesleft,
				 &outbuf, &outbytesleft);
	      if (iconv_ret == (size_t) -1 || inbytesleft != 0)
		{
		  if (errno == E2BIG)
		    {
		      ret_alloc *= 2;
		      identifier_to_locale_free (ret);
		      ret = NULL;
		      continue;
		    }
		  else
		    {
		      conversion_ok = false;
		      break;
		    }
		}
	      else if (iconv_ret != 0)
		{
		  conversion_ok = false;
		  break;
		}
	      /* Return to initial shift state.  */
	      if (iconv (cd, 0, 0, &outbuf, &outbytesleft) == (size_t) -1)
		{
		  if (errno == E2BIG)
		    {
		      ret_alloc *= 2;
		      identifier_to_locale_free (ret);
		      ret = NULL;
		      continue;
		    }
		  else
		    {
		      conversion_ok = false;
		      break;
		    }
		}
	      *outbuf = 0;
	      break;
	    }
	  iconv_close (cd);
	  if (conversion_ok)
	    return ret;
	}
    }
#endif

  /* Otherwise, convert non-ASCII characters in IDENT to UCNs.  */
  {
    char *ret = (char *) identifier_to_locale_alloc (10 * idlen + 1);
    char *p = ret;
    for (i = 0; i < idlen;)
      {
	unsigned int c;
	size_t utf8_len = pp_decode_utf8_char (&uid[i], idlen - i, &c);
	if (utf8_len == 1)
	  *p++ = uid[i];
	else
	  {
	    sprintf (p, "\\U%08x", c);
	    p += 10;
	  }
	i += utf8_len;
      }
    *p = 0;
    return ret;
  }
}
