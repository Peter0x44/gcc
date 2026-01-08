/* PR c/123437 */
/* { dg-do compile } */
/* { dg-options "-O2" } */

enum foo { F };
typedef enum foo vec_foo __attribute__((vector_size(16)));

vec_foo div(vec_foo x, vec_foo y)
{
  return x / y;
}
