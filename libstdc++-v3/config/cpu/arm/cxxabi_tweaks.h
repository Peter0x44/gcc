// Control various target specific ABI tweaks.  ARM version.

// Copyright (C) 2004-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file bits/cxxabi_tweaks.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{cxxabi.h}
 */

#ifndef _CXXABI_TWEAKS_H
#define _CXXABI_TWEAKS_H 1

#ifdef __cplusplus
namespace __cxxabiv1
{
  extern "C"
  {
#endif

#ifdef __ARM_EABI__
  // The ARM EABI uses the least significant bit of a 32-bit
  // guard variable.
#define _GLIBCXX_GUARD_TEST(x) ((*(x) & 1) != 0)
#define _GLIBCXX_GUARD_SET(x) *(x) = 1
#define _GLIBCXX_GUARD_BIT 1
#define _GLIBCXX_GUARD_PENDING_BIT __guard_test_bit (1, 1)
#define _GLIBCXX_GUARD_WAITING_BIT __guard_test_bit (2, 1)
  typedef int __guard;

#define _GLIBCXX_GUARD_TEST_AND_ACQUIRE(x) \
  ((__atomic_load_n(x, __ATOMIC_ACQUIRE) & 1) != 0)
#define _GLIBCXX_GUARD_SET_AND_RELEASE(x) \
  __atomic_store_n(x, 1, __ATOMIC_RELEASE)

  // We also want the element size in array cookies.
#define _GLIBCXX_ELTSIZE_IN_COOKIE 1

  // __cxa_vec_ctor should return a pointer to the array.
  typedef void * __cxa_vec_ctor_return_type;
#define _GLIBCXX_CXA_VEC_CTOR_RETURN(x) return x
  // Constructors and destructors return the "this" pointer.
  typedef void * __cxa_cdtor_return_type;

#else // __ARM_EABI__

  // The generic ABI uses the first byte of a 64-bit guard variable.
#define _GLIBCXX_GUARD_TEST(x) (*(char *) (x) != 0)
#define _GLIBCXX_GUARD_SET(x) *(char *) (x) = 1
#define _GLIBCXX_GUARD_BIT __guard_test_bit (0, 1)
#define _GLIBCXX_GUARD_PENDING_BIT __guard_test_bit (1, 1)
#define _GLIBCXX_GUARD_WAITING_BIT __guard_test_bit (2, 1)
  __extension__ typedef int __guard __attribute__((mode (__DI__)));

  // __cxa_vec_ctor has void return type.
  typedef void __cxa_vec_ctor_return_type;
#define _GLIBCXX_CXA_VEC_CTOR_RETURN(x) return
  // Constructors and destructors do not return a value.
  typedef void __cxa_cdtor_return_type;

#endif //!__ARM_EABI__

#ifdef __cplusplus
  }
} // namespace __cxxabiv1
#endif

#endif
