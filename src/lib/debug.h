#ifndef HEADER_DEBUG_H
#define HEADER_DEBUG_H 1

#if __GNUC__ > 1
#define ATTRIBUTE(X) __attribute__ (X)
#else
#define ATTRIBUTE(X)
#endif

#define UNUSED ATTRIBUTE ((unused))
#define NO_RETURN ATTRIBUTE ((noreturn))
#define PRINTF_FORMAT(FMT, FIRST) ATTRIBUTE ((format (printf, FMT, FIRST)))
#define SCANF_FORMAT(FMT, FIRST) ATTRIBUTE ((format (scanf, FMT, FIRST)))

void panic (const char *, ...)
     __attribute__ ((format (printf, 1, 2), noreturn));
void backtrace (void);

#endif /* debug.h */

/* This is outside the header guard so that debug.h may be
   included multiple times with different settings of NDEBUG. */
#undef ASSERT
#undef NOT_REACHED

#ifndef NDEBUG
#define ASSERT(CONDITION)                                               \
        if (CONDITION) {                                                \
                /* Nothing. */                                          \
        } else {                                                        \
                panic ("%s:%d: %s(): assertion `%s' failed.",           \
                       __FILE__, __LINE__, __func__, #CONDITION);       \
        }
#define NOT_REACHED() ASSERT (0)
#else
#define ASSERT(CONDITION) ((void) 0)
#define NOT_REACHED() for (;;)
#endif
