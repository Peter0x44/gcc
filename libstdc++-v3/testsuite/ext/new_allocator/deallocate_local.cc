//
// Copyright (C) 2004-2025 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <string>
#include <ext/new_allocator.h>
#include <replacement_memory_operators.h>

typedef char char_t;
typedef std::char_traits<char_t> traits_t;
typedef __gnu_cxx::new_allocator<char_t> allocator_t;
typedef std::basic_string<char_t, traits_t, allocator_t> string_t;  

int main()
{
  __gnu_test::counter::scope s;
  {
    string_t s;
    s += "bayou bend";
  }

  if (__gnu_test::counter::count() != 0)
    throw std::runtime_error("count not zero");

  return 0;
}
