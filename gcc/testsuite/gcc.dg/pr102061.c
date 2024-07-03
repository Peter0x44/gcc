/* { dg-do compile } */
/* { dg-options "-Wall -O2" } */
/* { dg-message "inlined from 'bar'" "" { target *-*-* } 0 } */
/* { dg-excess-errors "" } */

static inline void
foo (char *p)
{
  __builtin___memcpy_chk (p, "abc", 3, __builtin_object_size (p, 0));
}
static void
bar (char *p) __attribute__((noinline));
static void
bar (char *p)
{
  foo (p);
}
void f(char*) __attribute__((noipa));
char buf[2];
void
baz (void) __attribute__((noinline));
void
baz (void)
{
  bar (buf);
  f(buf);
}

void f(char*)
{}

int main(void)
{
    baz();
}
