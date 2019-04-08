#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#define Q 14
#define F (1 << Q)

#define INT_TO_FP(n) ((n) << Q)
#define FP_TO_INT(x) ((x) >> Q)
#define FP_TO_NEAREST_INT(x) ((x) >= 0  ? FP_TO_INT((x) + (F >> 1)) : \
                              FP_TO_INT((x) - (F >> 1)))
#define FP_MUL(x, y) ((int) ((((int64_t) (x)) * (y)) >> Q))
#define FP_DIV(x, y) ((int) ((((int64_t) (x)) << Q) / (y)))

#endif  /* threads/fixed-point.h */
