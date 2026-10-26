/* PR middle-end/21161 */
/* Warn about variables that are changed after setjmp returns and used
   after it returns again.  */
/* { dg-do compile } */
/* { dg-options "-Wclobbered" } */
/* { dg-skip-if "" { ! nonlocal_goto } } */
/* { dg-skip-if "no warnings without expansion" { *-*-* } { "-O0" "-flto" } { "" } } */

#include <setjmp.h>

extern int bar (void);
extern void baz (void *);
extern int vfork (void);
extern void _exit (int);

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

/* The assignment after setjmp only matters after longjmp, so it is dead
   as far as RTL is concerned, but y is still changed.  */

int
f5 (int c)
{
  jmp_buf env;
  int y = 1;	/* { dg-warning "might be clobbered" } */
  if (c)
    y = bar ();
  if (setjmp (env))
    return y;
  y = bar ();
  baz (&env);
  return y;
}

/* y is changed before setjmp is called again, which may be followed by a
   longjmp to an earlier *p.  */

int
f6 (jmp_buf *p)
{
  int y = 0;	/* { dg-warning "might be clobbered" } */
  for (;;)
    {
      if (setjmp (*p))
	return y;
      baz (p);
      y++;
      p++;
    }
}

int
f7 (int x)
{
  jmp_buf env;
  int y = 0;	/* { dg-warning "might be clobbered" } */
  if (setjmp (env))
    return y;
  y = bar ();
  baz (&env);
  return y;
}

int
f8 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-warning "might be clobbered" } */
  if (setjmp (env))
    {
      bar ();
      return y;
    }
  y = bar ();
  baz (&env);
  return y;
}

int
f9 (void)
{
  jmp_buf envs[2];
  int y = 0;	/* { dg-warning "might be clobbered" } */
  for (int i = 0; i < 2; i++)
    {
      if (setjmp (envs[i]))
	return y;
      baz (envs);
      y++;
    }
  return y;
}

int
f10 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-warning "might be clobbered" } */
  switch (setjmp (env))
    {
    case 0:
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (&env);
      return 0;
    case 1:
      return y;
    default:
      return 0;
    }
}

int
f11 (int x)
{
  int y = x;	/* { dg-warning "might be clobbered" } */
  if (vfork () == 0)
    {
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (0);
      _exit (y);
    }
  return y;
}

/* An uninitialized variable is changed after setjmp.  */

int
f12 (void)
{
  jmp_buf env;
  int y;	/* { dg-warning "might be clobbered" } */
  if (setjmp (env))
    return y;
  y = bar ();
  baz (&env);
  return y;
}
