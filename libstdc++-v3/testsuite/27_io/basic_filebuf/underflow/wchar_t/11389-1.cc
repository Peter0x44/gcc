// { dg-require-namedlocale "en_US.UTF-8" }

// Copyright (C) 2003-2025 Free Software Foundation, Inc.
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

// 27.8.1.4 Overridden virtual functions

#include <fstream>
#include <testsuite_hooks.h>

const char name_01[] = "tmp_11389-1";

void test01()
{
  using namespace std;

  filebuf fbout;
  fbout.open(name_01, ios_base::out);
  fbout.sputc('a');
  fbout.close();
  
  wfilebuf fbin;
  locale loc(locale("en_US.UTF-8"));
  fbin.pubimbue(loc);
  fbin.open(name_01, ios_base::in);
  VERIFY( fbin.sbumpc() == L'a' );
  VERIFY( fbin.sgetc() == wfilebuf::traits_type::eof() );
  fbin.close();
}

int main()
{
  test01();
  return 0;
}
