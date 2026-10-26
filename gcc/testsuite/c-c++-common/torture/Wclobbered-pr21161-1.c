/* PR middle-end/21161 */
/* Do not warn about variables that cannot be clobbered by longjmp, because
   they are not used after setjmp returns again, or because they are not
   changed after setjmp returns.  */
/* { dg-do compile } */
/* { dg-options "-Wclobbered" } */
/* { dg-skip-if "" { ! nonlocal_goto } } */
/* { dg-skip-if "no warnings without expansion" { *-*-* } { "-O0" "-flto" } { "" } } */

#include <setjmp.h>

extern int bar (void);
extern void baz (void *);
extern int vfork (void);
extern void _exit (int);

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
  char const **names = dir_read ();	/* { dg-bogus "clobbered" } */
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

/* The result of setjmp is tested for being zero.  */

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

/* The result of setjmp is used in a switch.  */

int
f7 (int x)
{
  jmp_buf env;
  int y = x;	/* { dg-bogus "clobbered" } */
  switch (setjmp (env))
    {
    case 0:
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (&env);
      return y;
    default:
      return 0;
    }
}

/* The parent does not use what the child of vfork changes.  */

int
f8 (int x)
{
  int y = x;	/* { dg-bogus "clobbered" } */
  if (vfork () == 0)
    {
      for (int i = 0; i < x; i++)
	y += bar ();
      baz (0);
      _exit (y);
    }
  return 0;
}

/* PR middle-end/61118: what glibc's pthread_cleanup_push expands to, in a
   loop.  */

struct cancel_jmp_buf { long buf[8]; int mask_was_saved; };
typedef struct { struct cancel_jmp_buf cancel_jmp_buf[1]; void *pad[4]; }
  unwind_buf_t;
extern int sigsetjmp_cancel (struct cancel_jmp_buf env[1], int savemask)
  __asm__ ("__sigsetjmp") __attribute__ ((__returns_twice__));
extern void unwind_next (unwind_buf_t *) __attribute__ ((__noreturn__));
extern void register_cancel (unwind_buf_t *);
extern void unregister_cancel (unwind_buf_t *);
extern void cleanup_fn (void *);
extern void wait_for (int *);

void
f9 (int *mutex, int *progress)
{
  while (*progress < 100)
    {
      unwind_buf_t cancel_buf;
      void (*cancel_routine) (void *) = cleanup_fn;	/* { dg-bogus "clobbered" } */
      void *cancel_arg = mutex;	/* { dg-bogus "clobbered" } */
      int not_first_call = sigsetjmp_cancel (cancel_buf.cancel_jmp_buf, 0);
      if (__builtin_expect (not_first_call, 0))
	{
	  cancel_routine (cancel_arg);
	  unwind_next (&cancel_buf);
	}
      register_cancel (&cancel_buf);
      wait_for (mutex);
      unregister_cancel (&cancel_buf);
    }
}

/* PR middle-end/65041: a2 is only set before setjmp.  */

extern void somefunc (int);
extern void someotherfunc (int);
extern int open_file (void);
extern void close_file (int);

void
f10 (int a1)
{
  jmp_buf jbuf;
  int fd;	/* { dg-warning "might be clobbered" } */
  int a2;	/* { dg-bogus "clobbered" } */
  if (a1 > 0)
    a2 = 42;
  else
    a2 = 43;
  fd = open_file ();
  if (setjmp (jbuf) == 0)
    {
      somefunc (a2);
      close_file (fd);
      fd = -1;
      someotherfunc (a2);
    }
  else if (fd >= 0)
    close_file (fd);
}
