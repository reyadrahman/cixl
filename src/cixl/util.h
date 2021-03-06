#ifndef CX_UTIL_H
#define CX_UTIL_H

#include <stdarg.h>

#define cx_min(x, y) ({				\
      typeof(x) _x = x;				\
      typeof(y) _y = y;				\
      (_x <= _y) ? _x : _y;			\
    })						\

#define cx_max(x, y) ({				\
      typeof(x) _x = x;				\
      typeof(y) _y = y;				\
      (_x >= _y) ? _x : _y;			\
    })

#define cx_baseof(ptr, typ, fld) ({				\
      const typeof( ((typ *)0)->fld ) *fp = (ptr);		\
      (typ *)((char *)fp - offsetof(typ, fld));			\
    })								\

char *cx_vfmt(const char *spec, va_list args);
char *cx_fmt(const char *spec, ...);

#endif
