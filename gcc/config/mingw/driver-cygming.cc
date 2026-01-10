/* Cygwin/MinGW specific driver functions.
   Copyright (C) 2026 Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#define IN_TARGET_CODE 1

#include "config.h"
#include <stdio.h>

/* Spec function to check if a .res file is actually a Microsoft .res format
   that needs windres processing, or a COFF object file that should be passed
   through as-is. See PR123504. */

const char *
cygming_check_res_file (int argc, const char **argv)
{
  FILE *f;
  unsigned char header[2];
  const char *filename;

  if (argc != 1)
    return NULL;

  filename = argv[0];

  f = fopen (filename, "rb");
  if (!f)
    {
      /* Can't open - let windres try and report its own error */
      return "%{!E:%{!M:%{!MM:windres -J res -O coff "
             "%{m32:-F pe-i386;m64|!m32:-F pe-x86-64} "
             "%{c:%W{o*}%{!o*:-o %w%b%O}}%{!c:-o %d%w%u%O} %i}}}";
    }

  /* Read first 2 bytes to check for COFF magic number */
  if (fread (header, 1, 2, f) != 2)
    {
      fclose (f);
      /* Can't read - let windres try */
      return "%{!E:%{!M:%{!MM:windres -J res -O coff "
             "%{m32:-F pe-i386;m64|!m32:-F pe-x86-64} "
             "%{c:%W{o*}%{!o*:-o %w%b%O}}%{!c:-o %d%w%u%O} %i}}}";
    }

  fclose (f);

  /* Check for COFF magic numbers:
     0x014C = IMAGE_FILE_MACHINE_I386
     0x8664 = IMAGE_FILE_MACHINE_AMD64
     0xAA64 = IMAGE_FILE_MACHINE_ARM64 */
  unsigned short magic = header[0] | (header[1] << 8);

  if (magic == 0x014C || magic == 0x8664 || magic == 0xAA64)
    return ""; /* COFF object - pass through as-is */

  /* Not COFF - assume it's a .res file and process with windres */
  return "%{!E:%{!M:%{!MM:windres -J res -O coff "
         "%{m32:-F pe-i386;m64|!m32:-F pe-x86-64} "
         "%{c:%W{o*}%{!o*:-o %w%b%O}}%{!c:-o %d%w%u%O} %i}}}";
}
