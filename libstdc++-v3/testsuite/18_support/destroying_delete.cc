// Copyright (C) 2019-2025 Free Software Foundation, Inc.
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

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// { dg-do run { target c++20 } }
// { dg-add-options no_pch }

#include <new>
#include <type_traits>
#include <testsuite_hooks.h>

#ifndef __cpp_lib_destroying_delete
# error "Feature-test macro for destroying_delete missing"
#elif __cpp_lib_destroying_delete != 201806L
# error "Feature-test macro for destroying_delete has wrong value"
#endif

const std::destroying_delete_t d{};
static_assert(std::is_same_v<decltype(d), decltype(std::destroying_delete)>);

struct X
{
  static bool called;

  void operator delete(X* p, std::destroying_delete_t)
  {
    called = true;
    p->~X();
    ::operator delete(p);
  }
};

bool X::called = false;

void
test01()
{
  X* x = new X;
  delete x;
  VERIFY( X::called );
}

int main()
{
  test01();
}
