/* { dg-require-effective-target vect_uintfloat_cvt } */
/* { dg-require-effective-target vect_float } */

#include <stdarg.h>
#include "tree-vect.h"

#define N 64 

unsigned int in[N*8] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};

int
main1 ()
{
  int i;
  float out2[N*8], fa[N*4];

  for (i = 0; i < N; i++)
    {
      out2[i*2] = (float) (in[i*2] * 2 + 11) ;
      out2[i*2 + 1] = (float) (in[i*2 + 1] * 3 + 7);
    
      fa[i] = (float) in[i*2+1];
    }

  /* check results:  */
#pragma GCC novector
  for (i = 0; i < N; i++)
    {
      if (out2[i*2] !=  (float) (in[i*2] * 2 + 11)
         || out2[i*2 + 1] != (float) (in[i*2 + 1] * 3 + 7)
         || fa[i] != (float) in[i*2+1])
        abort ();
    }


  return 0;
}

int main (void)
{
  check_vect ();

  main1 ();

  return 0;
}

/* { dg-final { scan-tree-dump-times "vectorized 1 loops" 1 "vect"  { target { vect_strided2 && vect_int_mult } } } } */
/* { dg-final { scan-tree-dump-times "vectorized 0 loops" 1 "vect"  { target { ! { vect_strided2 && vect_int_mult } } } } } */
/* { dg-final { scan-tree-dump-times "vectorizing stmts using SLP" 2 "vect"  { target { vect_strided2 && vect_int_mult } } } } */
/* { dg-final { scan-tree-dump-times "vectorizing stmts using SLP" 0 "vect"  { target { ! { vect_strided2 && vect_int_mult } } } } } */
  
