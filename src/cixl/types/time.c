#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "cixl/box.h"
#include "cixl/cx.h"
#include "cixl/types/time.h"

struct cx_time *cx_time_init(struct cx_time *time, int32_t months, int64_t ns) {
  time->months = months;
  time->ns = ns;
  return time;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  struct cx_time *xt = &x->as_time, *yt = &y->as_time;
  return xt->months == yt->months && xt->ns == yt->ns;
}

static bool ok_imp(struct cx_box *v) {
  struct cx_time *t = &v->as_time;
  return t->months || t->ns;
}

static void fprint_ns(int64_t ns, FILE *out) {
  int32_t h = ns / CX_HOUR;
  ns %= CX_HOUR;
  int32_t m = ns / CX_MIN;
  ns %= CX_MIN;
  int32_t s = ns / CX_SEC;
  ns %= CX_SEC;
  
  fprintf(out, "%" PRId32 ":%" PRId32 ":%" PRId32 ".%" PRId64, h, m, s, ns);
}

static void fprint_imp(struct cx_box *v, FILE *out) {
  fputs("Time(", out);
  struct cx_time *t = &v->as_time;
  
  if (t->months) {
    int32_t y = t->months / 12, m = t->months % 12, d = t->ns / CX_DAY; 
    fprintf(out, "%" PRId32 "/%" PRId32 "/%" PRId32, y, m, d);
    int64_t ns = t->ns % CX_DAY;

    if (ns) {
      fputc(' ', out);
      fprint_ns(ns, out);
    }
  } else {
    fprint_ns(t->ns, out);
  }

  fputc(')', out);
}

struct cx_type *cx_init_time_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Time", cx->any_type);
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->fprint = fprint_imp;
  return t;
}

