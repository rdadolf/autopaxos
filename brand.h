#ifndef _brand_h_
#define _brand_h_

#include <stdint.h>

// Thanks go to a few smart mathematician friends from a past life.

#define _ZERO64       UINT64_C(0)
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64-(x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64-(x))))


#define _BR_RUNUP_      INT64_C(128)
#define _BR_LG_TABSZ_   INT64_C(7)
#define _BR_TABSZ_      (INT64_C(1)<<_BR_LG_TABSZ_)

typedef struct {
  uint64_t hi, lo, ind;
  uint64_t tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) {\
  uint64_t x;\
  x = H ^ (H << A) ^ (L >> (64-A));\
  H = L | (x >> (B-64));\
  L = x << (128 - B);\
}

static inline uint64_t brand (brand_t *p) {
  uint64_t hi=p->hi, lo=p->lo, i=p->ind, ret;

  ret = p->tab[i];

  _BR_64STEP_(hi,lo,45,118);

  p->tab[i] = ret + hi; 

  p->hi  = hi;
  p->lo  = lo;
  p->ind = hi & _maskr(_BR_LG_TABSZ_);

  return ret;
}

static inline void brand_init (brand_t *p, uint64_t val)

{
  int64_t i;
  uint64_t hi, lo;

  hi = UINT64_C(0x9ccae22ed2c6e578) ^ val;
  lo = UINT64_C(0xce4db5d70739bd22) & _maskl(118-64);

  for (i = 0; i < 64; i++)
    _BR_64STEP_(hi,lo,33,118);

  for (i = 0; i < _BR_TABSZ_; i++) {
    _BR_64STEP_(hi,lo,33,118);
    p->tab[i] = hi;
  }
  p->ind = _BR_TABSZ_/2;
  p->hi  = hi;
  p->lo  = lo;

  for (i = 0; i < _BR_RUNUP_; i++)
    brand(p);
}

#endif
