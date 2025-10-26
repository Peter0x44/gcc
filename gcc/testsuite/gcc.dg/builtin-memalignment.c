/* { dg-do run } */
/* { dg-options "-O2 -fdump-tree-optimized" } */

extern void abort (void);

static int call_count = 0;
static int side_effect_var;

static int *get_ptr_with_side_effect (void)
{
  call_count++;
  return &side_effect_var;
}

void test_alignment (void *ptr, __SIZE_TYPE__ expected)
{
  __SIZE_TYPE__ result = __builtin_memalignment (ptr);
  if (result != expected)
    abort ();
}

/* Verify that constant folding happens */
void test_constant_p (void)
{
  /* These should be compile-time constants */
  if (!__builtin_constant_p (__builtin_memalignment ((void *)16)))
    abort ();
  if (!__builtin_constant_p (__builtin_memalignment ((void *)24)))
    abort ();
  if (!__builtin_constant_p (__builtin_memalignment ((void *)0)))
    abort ();
}

int main (void)
{
  /* Test with null pointer - alignment should be 0 */
  test_alignment ((void *)0, 0);

  /* Test powers of 2 alignments */
  test_alignment ((void *)1, 1);
  test_alignment ((void *)2, 2);
  test_alignment ((void *)4, 4);
  test_alignment ((void *)8, 8);
  test_alignment ((void *)16, 16);
  test_alignment ((void *)32, 32);
  test_alignment ((void *)64, 64);
  test_alignment ((void *)128, 128);
  test_alignment ((void *)256, 256);

  /* Test non-power-of-2 addresses */
  test_alignment ((void *)3, 1);   /* 0b11 -> alignment 1 */
  test_alignment ((void *)5, 1);   /* 0b101 -> alignment 1 */
  test_alignment ((void *)6, 2);   /* 0b110 -> alignment 2 */
  test_alignment ((void *)12, 4);  /* 0b1100 -> alignment 4 */
  test_alignment ((void *)24, 8);  /* 0b11000 -> alignment 8 */
  test_alignment ((void *)48, 16); /* 0b110000 -> alignment 16 */
  
  /* Verify constant folding happens */
  test_constant_p ();

  /* Test with variables having different alignments */
  char c __attribute__((aligned(1)));
  short s __attribute__((aligned(2)));
  int i __attribute__((aligned(4)));
  long long ll __attribute__((aligned(8)));
  char a16 __attribute__((aligned(16)));
  char a32 __attribute__((aligned(32)));
  char a64 __attribute__((aligned(64)));
  
  __SIZE_TYPE__ align_c = __builtin_memalignment(&c);
  __SIZE_TYPE__ align_s = __builtin_memalignment(&s);
  __SIZE_TYPE__ align_i = __builtin_memalignment(&i);
  __SIZE_TYPE__ align_ll = __builtin_memalignment(&ll);
  __SIZE_TYPE__ align_a16 = __builtin_memalignment(&a16);
  __SIZE_TYPE__ align_a32 = __builtin_memalignment(&a32);
  __SIZE_TYPE__ align_a64 = __builtin_memalignment(&a64);
  
  /* The alignment should be at least what we requested */
  if (align_c < 1)
    abort();
  if (align_s < 2)
    abort();
  if (align_i < 4)
    abort();
  if (align_ll < 8)
    abort();
  if (align_a16 < 16)
    abort();
  if (align_a32 < 32)
    abort();
  if (align_a64 < 64)
    abort();
  
  /* Test array elements */
  int array[16] __attribute__((aligned(64)));
  
  /* First element should have the array's alignment */
  if (__builtin_memalignment(&array[0]) < 64)
    abort();
  
  /* Other elements depend on their offset */
  /* array[1] is at offset 4, so alignment is 4 */
  if (__builtin_memalignment(&array[1]) != 4)
    abort();
  
  /* array[2] is at offset 8, so alignment is 8 */
  if (__builtin_memalignment(&array[2]) != 8)
    abort();
  
  /* array[4] is at offset 16, so alignment is 16 */
  if (__builtin_memalignment(&array[4]) != 16)
    abort();

  /* Test that side effects occur exactly once */
  call_count = 0;
  __SIZE_TYPE__ align = __builtin_memalignment(get_ptr_with_side_effect());
  if (call_count != 1)
    abort();  /* Side effect should happen exactly once */
  if (align < 4)
    abort();
  
  /* Test with post-increment */
  int *p = &array[0];
  align = __builtin_memalignment(p++);
  if (p != &array[1])
    abort();  /* Post-increment should happen */
  if (align < 64)
    abort();

  return 0;
}

/* Verify that the builtin is eliminated and replaced with the inline computation */
/* { dg-final { scan-tree-dump-not "__builtin_memalignment" "optimized" } } */
