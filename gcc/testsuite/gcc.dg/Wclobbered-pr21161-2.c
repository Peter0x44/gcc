/* PR middle-end/21161 */
/* Still warn about variables that are modified after setjmp and used
   after it returns nonzero.  */
/* { dg-do compile } */
/* { dg-options "-O2 -Wclobbered" } */
/* { dg-skip-if "" { ! nonlocal_goto } } */

#include <setjmp.h>

extern int bar (void);
extern void baz (void *);

int
f1 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-warning "might be clobbered" } */
  if (setjmp (env))
    return y;
  for (int i = 0; i < x; i++)
    y += bar ();
  baz (&env);
  return y;
}

int
f2 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-warning "might be clobbered" } */
  if (setjmp (env) == 0)
    {
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (&env);
      return y;
    }
  return y + 1;
}

/* The result of setjmp is not tested right away.  */

int
f3 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-warning "might be clobbered" } */
  int r = setjmp (env);
  bar ();
  if (r)
    return y;
  for (int i = 0; i < x; i++)
    y += bar ();
  baz (&env);
  return y;
}

/* getcontext returns zero both times, so the zero path matters too.  */

extern int getcontext (void *);
char ctx[1024];

int
f4 (int x)
{
  int y = x;	/* { dg-warning "might be clobbered" } */
  if (getcontext (ctx))
    return 0;
  for (int i = 0; i < x; i++)
    y += bar ();
  baz (ctx);
  return y;
}
