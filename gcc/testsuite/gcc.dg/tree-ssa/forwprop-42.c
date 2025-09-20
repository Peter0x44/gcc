/* PR tree-optimization/99504 */
/* Test that aggregate assignments get folded to scalar MEM_REF operations */
/* { dg-do compile } */
/* { dg-options "-O2 -fdump-tree-forwprop1" } */

#include <stdint.h>

struct pixel_4 {
  uint8_t r, g, b, a;
};

struct pixel_8 {
  uint16_t r, g, b, a;
};

struct pixel_16 {
  uint32_t r, g, b, a;
};

struct pixel_32 {
  uint64_t r, g, b, a;
};

void test_4_bytes(struct pixel_4 *dest, struct pixel_4 *src)
{
  *dest = *src;
}

void test_8_bytes(struct pixel_8 *dest, struct pixel_8 *src)
{
  *dest = *src;
}

void test_16_bytes(struct pixel_16 *dest, struct pixel_16 *src)
{
  *dest = *src;
}

void test_32_bytes(struct pixel_32 *dest, struct pixel_32 *src)
{
  *dest = *src;
}

void copy_pixels(struct pixel_4 *dest, struct pixel_4 *src, int n)
{
  for (int i = 0; i < n; i++)
    dest[i] = src[i];
}

/* { dg-final { scan-tree-dump-times "MEM\\\[" 8 "forwprop1" } } */
/* Check that we generate scalar temporaries for the folded assignments */
/* { dg-final { scan-tree-dump "_\[0-9\]+ = MEM\\\[" "forwprop1" } } */
/* { dg-final { scan-tree-dump "MEM\\\[.*\] = _\[0-9\]+" "forwprop1" } } */