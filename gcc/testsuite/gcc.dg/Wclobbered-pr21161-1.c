/* PR middle-end/21161 */
/* Do not warn about variables that are not used after setjmp returns
   nonzero.  */
/* { dg-do compile } */
/* { dg-options "-O2 -Wclobbered" } */
/* { dg-skip-if "" { ! nonlocal_goto } } */

#include <setjmp.h>

extern int bar (void);
extern void baz (void *);

/* Testcase from comment 14.  */

typedef struct { long a; } *b;
b global_obj;

void
f1 (b obj)	/* { dg-bogus "clobbered" } */
{
  jmp_buf env;
  if (setjmp (env) != 0)
    obj = global_obj;
  if (obj->a)
    longjmp (env, 1);
}

/* Reduced from GNU diffutils (comment 26).  */

static jmp_buf failed_locale_specific_sorting;
char const **dir_read (void);
char *somefn (char const *);

char *
f2 (char const *dir, char const *file)
{
  char const *match = file;	/* { dg-bogus "clobbered" } */
  char const **names = dir_read ();
  if (names)
    {
      if (setjmp (failed_locale_specific_sorting))
	match = file;
      else
	for (char const **p = names; *p; p++)
	  match = *p;
    }
  return somefn (match);
}

/* Reduced from mksh (comment 4).  */

struct source { int foo; const char *str; } *source;
jmp_buf jb;
extern char *evalstr (void);
extern struct source *pushs (void);
extern void newenv (void);
extern void quitenv (void);
extern char *strdup (const char *);

int
f3 (const char *content, int sub, char **resbuf)	/* { dg-bogus "clobbered" } */
{
  const char *ccp;	/* { dg-bogus "clobbered" } */
  struct source *s, *osource;

  osource = source;
  newenv ();
  if (setjmp (jb))
    {
      source = osource;
      quitenv ();
      return -2;
    }
  if (sub)
    {
      s = pushs ();
      s->str = content;
      source = s;
      ccp = evalstr ();
      source = osource;
    }
  else
    ccp = content;

  *resbuf = strdup (ccp);
  return 0;
}

/* Same, but with the result of setjmp tested for being zero.  */

int
f4 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-bogus "clobbered" } */
  if (setjmp (env) == 0)
    {
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (&env);
      return y;
    }
  return 0;
}

/* Variables that are only assigned before setjmp cannot be clobbered.  */

int
f5 (int c)
{
  jmp_buf env;
  int y = 1;	/* { dg-bogus "clobbered" } */
  if (c)
    y = bar ();
  if (setjmp (env))
    return y;
  baz (&env);
  return 0;
}

int
f6 (int n)
{
  jmp_buf env;
  int y = 0;	/* { dg-bogus "clobbered" } */
  for (int i = 0; i < n; i++)
    y += bar ();
  if (setjmp (env))
    return y;
  baz (&env);
  return y + 1;
}
