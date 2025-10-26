/* Compile-time constant folding test for __builtin_memalignment */
/* { dg-do compile } */
/* { dg-options "-O2 -fdump-tree-optimized" } */

/* Test that constant expressions fold to constants */
__SIZE_TYPE__ test_const_16 (void)
{
  return __builtin_memalignment ((void *)16);
}

__SIZE_TYPE__ test_const_24 (void)
{
  return __builtin_memalignment ((void *)24);
}

__SIZE_TYPE__ test_const_0 (void)
{
  return __builtin_memalignment ((void *)0);
}

__SIZE_TYPE__ test_const_48 (void)
{
  return __builtin_memalignment ((void *)48);
}

/* Verify that all calls are folded to constants and the builtin is eliminated */
/* { dg-final { scan-tree-dump-not "__builtin_memalignment" "optimized" } } */
/* { dg-final { scan-tree-dump-times "return 16;" 2 "optimized" } } */
/* { dg-final { scan-tree-dump "return 8;" "optimized" } } */
/* { dg-final { scan-tree-dump "return 0;" "optimized" } } */
