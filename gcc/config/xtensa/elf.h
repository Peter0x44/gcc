/* Xtensa/Elf configuration.
   Derived from the configuration for GCC for Intel i386 running Linux.
   Copyright (C) 2001-2025 Free Software Foundation, Inc.

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

#define TARGET_SECTION_TYPE_FLAGS xtensa_multibss_section_type_flags

#undef ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "short unsigned int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE 16

#undef ASM_SPEC
#define ASM_SPEC \
 "%{mtext-section-literals:--text-section-literals} \
  %{mno-text-section-literals:--no-text-section-literals} \
  %{mtarget-align:--target-align} \
  %{mno-target-align:--no-target-align} \
  %{mlongcalls:--longcalls} \
  %{mno-longcalls:--no-longcalls} \
  %{mauto-litpools:--auto-litpools} \
  %{mno-auto-litpools:--no-auto-litpools} \
  %{mabi=windowed:--abi-windowed} \
  %{mabi=call0:--abi-call0}"

#undef LIB_SPEC
#define LIB_SPEC "-lc -lsim -lc -lhandlers-sim -lhal"

#undef STARTFILE_SPEC
#define STARTFILE_SPEC \
  "crt1-sim%O%s crt0%O%s crti%O%s crtbegin%O%s _vectors%O%s"

#undef ENDFILE_SPEC
#define ENDFILE_SPEC "crtend%O%s crtn%O%s"

#undef LINK_SPEC
#define LINK_SPEC \
 "%{shared:-shared} \
  %{!shared: \
    %{!static: \
      %{rdynamic:-export-dynamic} \
    %{static:-static}}} \
  %{mabi=windowed:--abi-windowed} \
  %{mabi=call0:--abi-call0}"

#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX	"."

/* Avoid dots for compatibility with VxWorks.  */
#undef NO_DOLLAR_IN_LABEL
#define NO_DOT_IN_LABEL

/* Do not force "-fpic" for this target.  */
#define XTENSA_ALWAYS_PIC 0

#undef DEBUGGER_REGNO

/* Search for headers in $tooldir/arch/include and for libraries and
   startfiles in $tooldir/arch/lib.  */
#define GCC_DRIVER_HOST_INITIALIZATION \
  do { \
    char *tooldir, *archdir; \
    tooldir = concat (tooldir_base_prefix, spec_machine, \
		      dir_separator_str, NULL); \
    if (!IS_ABSOLUTE_PATH (tooldir)) \
      tooldir = concat (standard_exec_prefix, spec_machine, dir_separator_str, \
			spec_version, dir_separator_str, tooldir, NULL); \
    archdir = concat (tooldir, "arch", dir_separator_str, NULL); \
    add_prefix (&startfile_prefixes, \
		concat (archdir, "lib", dir_separator_str, NULL), \
		"GCC", PREFIX_PRIORITY_LAST, 0, 1); \
    add_prefix (&include_prefixes, archdir, \
		"GCC", PREFIX_PRIORITY_LAST, 0, 0); \
  } while (0)
