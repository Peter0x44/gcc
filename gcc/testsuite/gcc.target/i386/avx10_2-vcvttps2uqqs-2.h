#include "avx10-helper.h"
#include <limits.h>
#include <string.h>

#define SRC_SIZE (AVX512F_LEN_HALF / 32)
#define SIZE (AVX512F_LEN / 64)
#include "avx512f-mask-type.h"

static void
CALC (float *s, unsigned long long *r)
{
  int i;

  for (i = 0; i < SRC_SIZE; i++)
    {
      if (s[i] > ULONG_MAX)
	r[i] = ULONG_MAX;
      else if (s[i] < 0)
	r[i] = 0;
      else
	r[i] = s[i];
    }
}

void
TEST (void)
{
  UNION_TYPE (AVX512F_LEN_HALF, ) s;
  UNION_TYPE (AVX512F_LEN, i_uq) res1, res2, res3;
  MASK_TYPE mask = MASK_VALUE;
  unsigned long long res_ref[SIZE] = { 0 }, res_ref2[SIZE] = { 0 };
  int i, sign = 1;

  for (i = 0; i < SRC_SIZE; i++)
    {
      s.a[i] = 1.23 * (i + 2) * sign;
      sign = -sign;
    }

  for (i = 0; i < SIZE; i++)
    res2.a[i] = DEFAULT_VALUE;

  res1.x = INTRINSIC (_cvtts_ps_epu64) (s.x);
  res2.x = INTRINSIC (_mask_cvtts_ps_epu64) (res2.x, mask, s.x);
  res3.x = INTRINSIC (_maskz_cvtts_ps_epu64) (mask, s.x);

  CALC (s.a, res_ref);
  memcpy(res_ref2, res_ref, sizeof(res_ref));

  if (UNION_CHECK (AVX512F_LEN, i_uq) (res1, res_ref))
    abort ();

  MASK_MERGE (i_uq) (res_ref, mask, SRC_SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_uq) (res2, res_ref))
    abort ();

  MASK_ZERO (i_uq) (res_ref, mask, SRC_SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_uq) (res3, res_ref))
    abort ();

#if AVX512F_LEN == 512
  for (i = 0; i < SIZE; i++)
    res2.a[i] = DEFAULT_VALUE;

  res1.x = INTRINSIC (_cvtts_roundps_epu64) (s.x, 8);
  res2.x = INTRINSIC (_mask_cvtts_roundps_epu64) (res2.x, mask, s.x, 8);
  res3.x = INTRINSIC (_maskz_cvtts_roundps_epu64) (mask, s.x, 8);

  if (UNION_CHECK (AVX512F_LEN, i_uq) (res1, res_ref2))
    abort ();

  MASK_MERGE (i_uq) (res_ref2, mask, SRC_SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_uq) (res2, res_ref2))
    abort ();

  MASK_ZERO (i_uq) (res_ref2, mask, SRC_SIZE);
  if (UNION_CHECK (AVX512F_LEN, i_uq) (res3, res_ref2))
    abort ();
#endif
}
