/* { dg-final { check-function-bodies "**" "" "-DCHECK_ASM" } } */

#include "test_sve_acle.h"

/*
** uzp1_mf8_tied1:
**	uzp1	z0\.b, z0\.b, z1\.b
**	ret
*/
TEST_UNIFORM_Z (uzp1_mf8_tied1, svmfloat8_t,
		z0 = svuzp1_mf8 (z0, z1),
		z0 = svuzp1 (z0, z1))

/*
** uzp1_mf8_tied2:
**	uzp1	z0\.b, z1\.b, z0\.b
**	ret
*/
TEST_UNIFORM_Z (uzp1_mf8_tied2, svmfloat8_t,
		z0 = svuzp1_mf8 (z1, z0),
		z0 = svuzp1 (z1, z0))

/*
** uzp1_mf8_untied:
**	uzp1	z0\.b, z1\.b, z2\.b
**	ret
*/
TEST_UNIFORM_Z (uzp1_mf8_untied, svmfloat8_t,
		z0 = svuzp1_mf8 (z1, z2),
		z0 = svuzp1 (z1, z2))
