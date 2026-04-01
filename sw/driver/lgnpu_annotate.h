/* lgnpu_annotate.h - Kernel-compatible subset of sw/shared/annotations.h
 *
 * The full annotations.h includes userspace headers (<string.h>, <assert.h>,
 * <stdint.h>) that are unavailable in kernel context.  This header provides
 * the same macro names backed by GCC built-ins available in the kernel.
 */

#ifndef _LGNPU_ANNOTATE_H
#define _LGNPU_ANNOTATE_H

/* Branch prediction hints */
#define LIKELY(cond)             __builtin_expect(!!(cond), 1)
#define UNLIKELY(cond)           __builtin_expect(!!(cond), 0)

/* Inlining control (no trailing inline keyword) */
#define ALWAYS_INLINE            __attribute__((always_inline))
#define NO_INLINE                __attribute__((noinline))

/* Hot / cold paths */
#define HOT_CALL                 __attribute__((hot))
#define COLD_CALL                __attribute__((cold))

/* Result checking */
#define NO_DISCARD               __attribute__((warn_unused_result))

/* Null-pointer annotations */
#define NO_NULL_ARGS             __attribute__((nonnull))
#define NON_NULL_ARGS(...)       __attribute__((nonnull(__VA_ARGS__)))

#endif /* _LGNPU_ANNOTATE_H */
