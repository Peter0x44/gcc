/*
 * Copyright (C) 2007-2026 Free Software Foundation, Inc.
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3, or (at your option) any
 * later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Under Section 7 of GPL version 3, you are granted additional
 * permissions described in the GCC Runtime Library Exception, version
 * 3.1, as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License and
 * a copy of the GCC Runtime Library Exception along with this program;
 * see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _SOFT_FLOAT
#if __PREC == 32
 #define X87CW		(0 << 8)	/* Single precision (24 bits) */
#elif __PREC == 64
 #define X87CW		(2 << 8)	/* Double precision (53 bits) */
#elif __PREC == 80
 #define X87CW		(3 << 8)	/* Extended precision (64 bits) */
#else
 #error "Wrong precision requested."
#endif

#define X87CW_PCMASK	(3 << 8)

static void __attribute__((constructor))
set_precision (void)
{
  unsigned short int cwd;

  asm volatile ("fnstcw\t%0" : "=m" (cwd));

  cwd &= ~X87CW_PCMASK;
  cwd |= X87CW;

  asm volatile ("fldcw\t%0" : : "m" (cwd));
}

#if defined (_WIN32) && defined (__i386__)
/* Windows does not let a new thread inherit the x87 control word of the
   thread that created it; every thread starts with 53-bit precision.  Set
   the precision again from a TLS callback, which runs on each new thread
   before its start routine.  This cannot work on x86_64, where the thread's
   initial context, including its floating-point state, is loaded after the
   TLS callbacks have run.  */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static void NTAPI
set_precision_tls (PVOID handle, DWORD reason, PVOID reserved)
{
  (void) handle;
  (void) reserved;
  if (reason == DLL_THREAD_ATTACH)
    set_precision ();
}

/* Pull in the CRT's TLS directory, so that the callback is run.  */
extern const IMAGE_TLS_DIRECTORY _tls_used;
static const void *const crtprec_tls_used __attribute__((used)) = &_tls_used;

/* The CRT's TLS callbacks run in the order of their .CRT$XL? section names.
   Use .CRT$XLB so that the precision is already set when the CRT's
   callback in .CRT$XLC runs dynamic initializers of thread-local
   variables.  */
static const PIMAGE_TLS_CALLBACK crtprec_tls_callback
  __attribute__((section (".CRT$XLB"), used)) = set_precision_tls;
#endif
#endif
