// { dg-do compile { target c++20 } }
// { dg-additional-options "-fconcepts" }
#include <type_traits>

template<typename t2, typename t = std::remove_reference_t<t2>>
concept IntegralOrIntegralRef = std::is_integral_v<t>;

template<IntegralOrIntegralRef t>
auto foo(t && v)
{
  return v;
}

int main()
{
  int i = 7;
  foo(8);
  return foo(i);
}
