/* { dg-do compile { target { ! ia32 } } } */
/* { dg-options "-O2 -mno-sse -mno-skip-rax-setup" } */

// For sysv abi, eax holds the number of XMM registers used in the call.
// Since sse is disabled, check that it is zeroed
/* { dg-final { scan-assembler-times "xorl\[\\t \]*\\\%eax,\[\\t \]*%eax" 2 } } */

// For ms abi, the argument should go in edx
/* { dg-final { scan-assembler-times "movl\[\\t \]*\\\$20,\[\\t \]*%edx" 2 } } */

// For sysv abi, the argument should go in esi
/* { dg-final { scan-assembler-times "movl\[\\t \]*\\\$20,\[\\t \]*%esi" 2 } } */

void
__attribute__((__sysv_abi__))
fooSys (const char *, ...);

void
test_sys1 (void)
{
  fooSys ("%d", 20);
}

int
test_sys2 (void)
{
  fooSys ("%d", 20);
  return 3;
}

void
__attribute__((__ms_abi__))
fooMs (const char *, ...);

void
test_ms1 (void)
{
  fooMs ("%d", 20);
}

int
test_ms2 (void)
{
  fooMs ("%d", 20);
  return 3;
}
