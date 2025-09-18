/* { dg-do run } */
/* { dg-options "-O2 -lm -fmath-errno" } */

#include <errno.h>

void test_double(void)
{
  double s, c;

  errno = 0;
  s = __builtin_sin(__builtin_inf());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  c = __builtin_cos(__builtin_inf());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  __builtin_sincos(__builtin_inf(), &s, &c);
  if (errno != EDOM)
    __builtin_abort();
}

void test_float(void)
{
  float s, c;

  errno = 0;
  s = __builtin_sinf(__builtin_inff());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  c = __builtin_cosf(__builtin_inff());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  __builtin_sincosf(__builtin_inff(), &s, &c);
  if (errno != EDOM)
    __builtin_abort();
}

void test_longdouble(void)
{
  long double s, c;

  errno = 0;
  s = __builtin_sinl(__builtin_infl());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  c = __builtin_cosl(__builtin_infl());
  if (errno != EDOM)
    __builtin_abort();

  errno = 0;
  __builtin_sincosl(__builtin_infl(), &s, &c);
  if (errno != EDOM)
    __builtin_abort();
}

int main(void)
{
  test_double();
  test_float();
  test_longdouble();
}