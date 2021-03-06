#include "cixl/box.h"
#include "cixl/cx.h"
#include "cixl/error.h"
#include "cixl/types/fimp.h"
#include "cixl/types/func.h"
#include "cixl/types/vect.h"
#include "cixl/scope.h"
#include "cixl/vec.h"

struct cx_vect *cx_vect_new() {
  struct cx_vect *v = malloc(sizeof(struct cx_vect));
  cx_vec_init(&v->imp, sizeof(struct cx_box));
  v->nrefs = 1;
  return v;
}

struct cx_vect *cx_vect_ref(struct cx_vect *vect) {
  vect->nrefs++;
  return vect;
}

void cx_vect_unref(struct cx_vect *vect) {
  cx_test(vect->nrefs > 0);
  vect->nrefs--;

  if (!vect->nrefs) {
    cx_do_vec(&vect->imp, struct cx_box, b) { cx_box_deinit(b); }
    cx_vec_deinit(&vect->imp);
    free(vect);
  }
}

void cx_vect_fprint(struct cx_vec *imp, FILE *out) {
  fputc('[', out);
  char sep = 0;
  
  cx_do_vec(imp, struct cx_box, b) {
    if (sep) { fputc(sep, out); }
    cx_fprint(b, out);
    sep = ' ';
  }

  fputc(']', out);
}

static bool len_imp(struct cx_scope *scope) {
  struct cx_box vec = *cx_test(cx_pop(scope, false));
  struct cx_vect *v = vec.as_ptr;
  cx_box_init(cx_push(scope), scope->cx->int_type)->as_int = v->imp.count;
  cx_box_deinit(&vec);
  return true;
}

static bool push_imp(struct cx_scope *scope) {
  struct cx_box
    val = *cx_test(cx_pop(scope, false)),
    vec = *cx_test(cx_pop(scope, false));
  
  struct cx_vect *v = vec.as_ptr;
  *(struct cx_box *)cx_vec_push(&v->imp) = val;
  cx_box_deinit(&vec);
  return true;
}

static bool pop_imp(struct cx_scope *scope) {
  struct cx_box vec = *cx_test(cx_pop(scope, false));
  struct cx_vect *v = vec.as_ptr;
  *cx_push(scope) = *(struct cx_box *)cx_vec_pop(&v->imp);
  cx_box_deinit(&vec);
  return true;
}

static bool for_imp(struct cx_scope *scope) {
  struct cx_box
    act = *cx_test(cx_pop(scope, false)),
    vect = *cx_test(cx_pop(scope, false));
  
  struct cx_vect *v = vect.as_ptr;
  bool ok = false;
  
  cx_do_vec(&v->imp, struct cx_box, b) {
    cx_copy(cx_push(scope), b);
    if (!cx_call(&act, scope)) { goto exit; }
  }

  ok = true;
 exit:
  cx_box_deinit(&vect);
  cx_box_deinit(&act);
  return ok;
}

static bool map_imp(struct cx_scope *scope) {
  struct cx *cx = scope->cx;

  struct cx_box
    act = *cx_test(cx_pop(scope, false)),
    vect = *cx_test(cx_pop(scope, false));
  
  struct cx_vect *v = vect.as_ptr;
  bool ok = false;
  
  cx_do_vec(&v->imp, struct cx_box, in) {
    cx_copy(cx_push(scope), in);
    if (!cx_call(&act, scope)) { goto exit; }
    struct cx_box *out = cx_pop(scope, true);

    if (!out) {
      cx_error(cx, cx->row, cx->col, "Missing result");
      goto exit;
    }

    cx_box_deinit(in);
    *in = *out;
  }

  *cx_push(scope) = vect;
  ok = true;
 exit:
  cx_box_deinit(&act);
  return ok;
}

static bool equid_imp(struct cx_box *x, struct cx_box *y) {
  return x->as_ptr == y->as_ptr;
}

static bool eqval_imp(struct cx_box *x, struct cx_box *y) {
  struct cx_vect *xv = x->as_ptr, *yv = y->as_ptr;
  if (xv->imp.count != yv->imp.count) { return false; }
  
  for (size_t i = 0; i < xv->imp.count; i++) {
    if (!cx_eqval(cx_vec_get(&xv->imp, i), cx_vec_get(&yv->imp, i))) { return false; }
  }
  
  return true;
}

static bool ok_imp(struct cx_box *b) {
  struct cx_vect *v = b->as_ptr;
  return v->imp.count;
}

static void copy_imp(struct cx_box *dst, struct cx_box *src) {
  dst->as_ptr = cx_vect_ref(src->as_ptr);
}

static void clone_imp(struct cx_box *dst, struct cx_box *src) {
  struct cx_vect *src_vect = src->as_ptr, *dst_vect = cx_vect_new();
  dst->as_ptr = dst_vect;

  cx_do_vec(&src_vect->imp, struct cx_box, v) {
    cx_clone(cx_vec_push(&dst_vect->imp), v);
  }
}

static void fprint_imp(struct cx_box *b, FILE *out) {
  struct cx_vect *v = b->as_ptr;
  cx_vect_fprint(&v->imp, out);
  fprintf(out, "@%d", v->nrefs);
}

static void deinit_imp(struct cx_box *v) {
  cx_vect_unref(v->as_ptr);
}

struct cx_type *cx_init_vect_type(struct cx *cx) {
  struct cx_type *t = cx_add_type(cx, "Vect", cx->any_type);
  t->eqval = eqval_imp;
  t->equid = equid_imp;
  t->ok = ok_imp;
  t->copy = copy_imp;
  t->clone = clone_imp;
  t->fprint = fprint_imp;
  t->deinit = deinit_imp;
  
  cx_add_func(cx, "len", cx_arg(t))->ptr = len_imp;
  cx_add_func(cx, "push", cx_arg(t), cx_arg(cx->any_type))->ptr = push_imp;
  cx_add_func(cx, "pop", cx_arg(t))->ptr = pop_imp;
  
  cx_add_func(cx, "for", cx_arg(t), cx_arg(cx->any_type))->ptr = for_imp;
  cx_add_func(cx, "map", cx_arg(t), cx_arg(cx->any_type))->ptr = map_imp;
  
  return t;
}
