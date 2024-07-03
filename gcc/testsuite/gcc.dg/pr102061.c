/* { dg-do compile } */
/* { dg-options "-Wall -O2" } */
/* { dg-message "In function 'foo'" "" { target *-*-* } 0 } */
/* { dg-message "inlined from 'bar'" "" { target *-*-* } 0 } */
/* { dg-message "isra" "" { xfail *-*-* } 0 } */
/* { dg-excess-errors "" } */

// The warnings generated here should not contain names of clones, like
// 'foo.isra' and 'bar.isra'

// Emit warning with "In function 'foo'"
__attribute__((noinline))
static int foo(char* p) {
    __builtin_strncpy(p, p, 1);
    return 0;
}

// Emit warning with "inlined from 'bar'"
// For some reason, this function needs to be infinite recursive
// for the warning to show up in an isra clone.
static int bar(char* p) {
    __builtin_strncpy(p, p, 1);
    bar(p);
    return 0;
}

void baz() {
    char c[0];
    foo(c);
    bar(c);
}
