// { dg-options "-DUSE_FILESYSTEM_TS -lstdc++fs" }
// { dg-do run { target c++11 } }
// { dg-require-filesystem-ts "" }

// Copyright (C) 2014-2025 Free Software Foundation, Inc.
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

// 8.4.9 path decomposition [path.decompose]

#include <experimental/filesystem>
#include <testsuite_hooks.h>
#include <testsuite_fs.h>

using std::experimental::filesystem::path;

void
test01()
{
  path p1 = "foo/bar";
  VERIFY( p1.root_path() == path() );
  path p2 = "/foo/bar";
  VERIFY( p2.root_path() == path("/") );
}

void
test02()
{
  for (const path p : __gnu_test::test_paths)
  {
    path rootp = p.root_path();
    path rootn = p.root_name();
    path rootd = p.root_directory();
    VERIFY( rootp == (rootn / rootd) );
  }
}

int
main()
{
  test01();
  test02();
}
