/* ******************************************************* */
/*                                                         */
/*   annotations.h                                         */
/*   License: MIT                                          */
/*                                                         */
/*   Author: https://github.com/Arty3                      */
/*   Repo:   https://github.com/Arty3/annotations          */
/*                                                         */
/* ******************************************************* */

/**
 * @file annotations.h
 * @brief Comprehensive cross-platform compiler annotations and attributes.
 *
 * Provides unified macros for compiler-specific attributes across:
 * - Languages: C and C++
 * - Compilers: GCC, Clang, MSVC
 * - Platforms: Windows, Linux, macOS
 * - Architectures: x86, x64, ARM32, ARM64
 *
 * Uses C++ standard attributes when available (with appropriate language
 * standard), otherwise falls back to compiler-specific extensions.
 *
 * @note Include this header after standard library headers to avoid conflicts.
 */

#if !defined(__ANNOTATIONS_H_)
#define __ANNOTATIONS_H_

#if !defined(ANNOTATIONS_REDEFINE)
/* Flipped for this project (opt-in) */
#define ANNOTATIONS_REDEFINE 1
#endif

#if (ANNOTATIONS_REDEFINE + 0)

#if defined(COMPILER_MSVC)
#undef COMPILER_MSVC
#endif
#if defined(COMPILER_CLANG)
#undef COMPILER_CLANG
#endif
#if defined(COMPILER_GCC)
#undef COMPILER_GCC
#endif
#if defined(COMPILER_VERSION)
#undef COMPILER_VERSION
#endif
#if defined(PLATFORM_WINDOWS)
#undef PLATFORM_WINDOWS
#endif
#if defined(PLATFORM_LINUX)
#undef PLATFORM_LINUX
#endif
#if defined(PLATFORM_MACOS)
#undef PLATFORM_MACOS
#endif
#if defined(PLATFORM_UNIX)
#undef PLATFORM_UNIX
#endif
#if defined(ARCH_X64)
#undef ARCH_X64
#endif
#if defined(ARCH_X86)
#undef ARCH_X86
#endif
#if defined(ARCH_ARM64)
#undef ARCH_ARM64
#endif
#if defined(ARCH_ARM32)
#undef ARCH_ARM32
#endif
#if defined(ARCH_64BIT)
#undef ARCH_64BIT
#endif
#if defined(ARCH_32BIT)
#undef ARCH_32BIT
#endif
#if defined(LANG_CPP)
#undef LANG_CPP
#endif
#if defined(LANG_C)
#undef LANG_C
#endif
#if defined(CPP_VERSION)
#undef CPP_VERSION
#endif
#if defined(C_VERSION)
#undef C_VERSION
#endif
#if defined(CPP11)
#undef CPP11
#endif
#if defined(CPP14)
#undef CPP14
#endif
#if defined(CPP17)
#undef CPP17
#endif
#if defined(CPP20)
#undef CPP20
#endif
#if defined(CPP23)
#undef CPP23
#endif
#if defined(C99)
#undef C99
#endif
#if defined(C11)
#undef C11
#endif
#if defined(C17)
#undef C17
#endif
#if defined(C23)
#undef C23
#endif
#if defined(HAS_CPP_ATTR)
#undef HAS_CPP_ATTR
#endif
#if defined(HAS_NS_ATTR)
#undef HAS_NS_ATTR
#endif
#if defined(STATIC_ASSERT)
#undef STATIC_ASSERT
#endif
#if defined(INLINE)
#undef INLINE
#endif
#if defined(FORCE_INLINE)
#undef FORCE_INLINE
#endif
#if defined(ALWAYS_INLINE)
#undef ALWAYS_INLINE
#endif
#if defined(NO_INLINE)
#undef NO_INLINE
#endif
#if defined(FLATTEN)
#undef FLATTEN
#endif
#if defined(NO_RETURN)
#undef NO_RETURN
#endif
#if defined(DISCARD)
#undef DISCARD
#endif
#if defined(NO_DISCARD)
#undef NO_DISCARD
#endif
#if defined(NODISCARD)
#undef NODISCARD
#endif
#if defined(MAYBE_UNUSED)
#undef MAYBE_UNUSED
#endif
#if defined(DEPRECATED)
#undef DEPRECATED
#endif
#if defined(FALLTHROUGH)
#undef FALLTHROUGH
#endif
#if defined(PURE)
#undef PURE
#endif
#if defined(PURE_CALL)
#undef PURE_CALL
#endif
#if defined(CONST_CALL)
#undef CONST_CALL
#endif
#if defined(HOT_CALL)
#undef HOT_CALL
#endif
#if defined(COLD_CALL)
#undef COLD_CALL
#endif
#if defined(MALLOC_CALL)
#undef MALLOC_CALL
#endif
#if defined(ALLOC_SIZE)
#undef ALLOC_SIZE
#endif
#if defined(ALLOC_ALIGN)
#undef ALLOC_ALIGN
#endif
#if defined(API_CALL)
#undef API_CALL
#endif
#if defined(API_IMPORT)
#undef API_IMPORT
#endif
#if defined(API_LOCAL)
#undef API_LOCAL
#endif
#if defined(LIKELY)
#undef LIKELY
#endif
#if defined(UNLIKELY)
#undef UNLIKELY
#endif
#if defined(EXPECT_TRUE)
#undef EXPECT_TRUE
#endif
#if defined(EXPECT_FALSE)
#undef EXPECT_FALSE
#endif
#if defined(COND_PROB)
#undef COND_PROB
#endif
#if defined(ASSUME)
#undef ASSUME
#endif
#if defined(UNREACHABLE)
#undef UNREACHABLE
#endif
#if defined(ALIGNED)
#undef ALIGNED
#endif
#if defined(ALIGN)
#undef ALIGN
#endif
#if defined(PACKED)
#undef PACKED
#endif
#if defined(NO_UNIQUE_ADDRESS)
#undef NO_UNIQUE_ADDRESS
#endif
#if defined(RESTRICT)
#undef RESTRICT
#endif
#if defined(NO_NULL_ARGS)
#undef NO_NULL_ARGS
#endif
#if defined(NON_NULL_ARGS)
#undef NON_NULL_ARGS
#endif
#if defined(RETURNS_NON_NULL)
#undef RETURNS_NON_NULL
#endif
#if defined(TLS_MODEL)
#undef TLS_MODEL
#endif
#if defined(THREAD_LOCAL)
#undef THREAD_LOCAL
#endif
#if defined(CAPABILITY)
#undef CAPABILITY
#endif
#if defined(GUARDED_BY)
#undef GUARDED_BY
#endif
#if defined(ACQUIRE_CAPABILITY)
#undef ACQUIRE_CAPABILITY
#endif
#if defined(RELEASE_CAPABILITY)
#undef RELEASE_CAPABILITY
#endif
#if defined(CONTRACT)
#undef CONTRACT
#endif
#if defined(COMPILER_BARRIER)
#undef COMPILER_BARRIER
#endif
#if defined(MEMORY_BARRIER)
#undef MEMORY_BARRIER
#endif
#if defined(LOAD_BARRIER)
#undef LOAD_BARRIER
#endif
#if defined(STORE_BARRIER)
#undef STORE_BARRIER
#endif
#if defined(CDECL)
#undef CDECL
#endif
#if defined(STDCALL)
#undef STDCALL
#endif
#if defined(FASTCALL)
#undef FASTCALL
#endif
#if defined(THISCALL)
#undef THISCALL
#endif
#if defined(VECTORCALL)
#undef VECTORCALL
#endif
#if defined(NO_STACK_PROTECT)
#undef NO_STACK_PROTECT
#endif
#if defined(NO_CFI)
#undef NO_CFI
#endif
#if defined(NO_SANITIZE)
#undef NO_SANITIZE
#endif
#if defined(NO_ASAN)
#undef NO_ASAN
#endif
#if defined(NO_TSAN)
#undef NO_TSAN
#endif
#if defined(NO_UBSAN)
#undef NO_UBSAN
#endif
#if defined(CONSTRUCTOR)
#undef CONSTRUCTOR
#endif
#if defined(DESTRUCTOR)
#undef DESTRUCTOR
#endif
#if defined(PREFETCH)
#undef PREFETCH
#endif
#if defined(PREFETCH_READ)
#undef PREFETCH_READ
#endif
#if defined(PREFETCH_WRITE)
#undef PREFETCH_WRITE
#endif
#if defined(PREFETCH_LOCALITY_NONE)
#undef PREFETCH_LOCALITY_NONE
#endif
#if defined(PREFETCH_LOCALITY_LOW)
#undef PREFETCH_LOCALITY_LOW
#endif
#if defined(PREFETCH_LOCALITY_MEDIUM)
#undef PREFETCH_LOCALITY_MEDIUM
#endif
#if defined(PREFETCH_LOCALITY_HIGH)
#undef PREFETCH_LOCALITY_HIGH
#endif
#if defined(ARRAY_SIZE)
#undef ARRAY_SIZE
#endif
#if defined(ARRAY_END)
#undef ARRAY_END
#endif
#if defined(UNUSED_VAR)
#undef UNUSED_VAR
#endif
#if defined(ASSUME_ALIGNED)
#undef ASSUME_ALIGNED
#endif
#if defined(OFFSET_OF)
#undef OFFSET_OF
#endif
#if defined(CONTAINER_OF)
#undef CONTAINER_OF
#endif
#if defined(MIN)
#undef MIN
#endif
#if defined(MAX)
#undef MAX
#endif
#if defined(CLAMP)
#undef CLAMP
#endif
#if defined(IS_POWER_OF_2)
#undef IS_POWER_OF_2
#endif
#if defined(ALIGN_UP)
#undef ALIGN_UP
#endif
#if defined(ALIGN_DOWN)
#undef ALIGN_DOWN
#endif
#if defined(IS_ALIGNED)
#undef IS_ALIGNED
#endif
#if defined(BIT)
#undef BIT
#endif
#if defined(BIT_SET)
#undef BIT_SET
#endif
#if defined(BIT_CLEAR)
#undef BIT_CLEAR
#endif
#if defined(BIT_TOGGLE)
#undef BIT_TOGGLE
#endif
#if defined(BIT_CHECK)
#undef BIT_CHECK
#endif
#if defined(STRINGIFY)
#undef STRINGIFY
#endif
#if defined(STRINGIFY_)
#undef STRINGIFY_
#endif
#if defined(CONCAT)
#undef CONCAT
#endif
#if defined(CONCAT_)
#undef CONCAT_
#endif
#if defined(DEBUG_BREAK)
#undef DEBUG_BREAK
#endif
#if defined(CPU_RELAX)
#undef CPU_RELAX
#endif
#if defined(ASSERT)
#undef ASSERT
#endif
#if defined(GUARANTEE)
#undef GUARANTEE
#endif
#if defined(HAS_SSE2)
#undef HAS_SSE2
#endif
#if defined(HAS_AVX2)
#undef HAS_AVX2
#endif
#if defined(HAS_NEON)
#undef HAS_NEON
#endif
#if defined(ROTL32)
#undef ROTL32
#endif
#if defined(ROTR32)
#undef ROTR32
#endif
#if defined(ROTL64)
#undef ROTL64
#endif
#if defined(ROTR64)
#undef ROTR64
#endif
#if defined(BSWAP16)
#undef BSWAP16
#endif
#if defined(BSWAP32)
#undef BSWAP32
#endif
#if defined(BSWAP64)
#undef BSWAP64
#endif
#if defined(CLZ32)
#undef CLZ32
#endif
#if defined(CLZ64)
#undef CLZ64
#endif
#if defined(CTZ32)
#undef CTZ32
#endif
#if defined(CTZ64)
#undef CTZ64
#endif
#if defined(POPCOUNT32)
#undef POPCOUNT32
#endif
#if defined(POPCOUNT64)
#undef POPCOUNT64
#endif
#if defined(FORMAT_PRINTF)
#undef FORMAT_PRINTF
#endif
#if defined(FORMAT_SCANF)
#undef FORMAT_SCANF
#endif
#if defined(memcpy_constexpr)
#undef memcpy_constexpr
#endif
#if defined(memset_constexpr)
#undef memset_constexpr
#endif
#if defined(WEAK_ALIAS)
#undef WEAK_ALIAS
#endif
#if defined(STRONG_ALIAS)
#undef STRONG_ALIAS
#endif
#if defined(IS_SAME_TYPE)
#undef IS_SAME_TYPE
#endif
#if defined(OFFSET_PTR)
#undef OFFSET_PTR
#endif
#if defined(PTR_DIFF)
#undef PTR_DIFF
#endif
#if defined(ENDIAN_LITTLE)
#undef ENDIAN_LITTLE
#endif
#if defined(ENDIAN_BIG)
#undef ENDIAN_BIG
#endif

#endif /* ANNOTATIONS_REDEFINE */

#if defined(_MSC_VER) && !defined(__clang__)
#define COMPILER_MSVC		1
#define COMPILER_VERSION	_MSC_VER
#elif defined(__clang__)
#define COMPILER_CLANG		1
#define COMPILER_VERSION	(__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(__GNUC__)
#define COMPILER_GCC		1
#define COMPILER_VERSION	(__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#else
#error "Unsupported compiler. Requires MSVC, Clang, or GCC."
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if !defined(__has_builtin)
#define __has_builtin(x)		0
#endif
#if !defined(__has_attribute)
#define __has_attribute(x)		0
#endif
#if !defined(__has_feature)
#define __has_feature(x)		0
#endif
#if !defined(__has_extension)
#define __has_extension(x)		0
#endif
#if !defined(__has_cpp_attribute)
#define __has_cpp_attribute(x)	0
#endif
#if !defined(__has_c_attribute)
#define __has_c_attribute(x)	0
#endif
#endif

#if defined(_WIN32) || defined(_WIN64) || defined(__WIN32__)
#define PLATFORM_WINDOWS		1
#elif defined(__linux__) || defined(__linux) || defined(linux)
#define PLATFORM_LINUX			1
#define PLATFORM_UNIX			1
#elif defined(__APPLE__) && defined(__MACH__)
#define PLATFORM_MACOS			1
#define PLATFORM_UNIX			1
#elif defined(__unix__) || defined(__unix)
#define PLATFORM_UNIX			1
#else
#error "Unsupported platform."
#endif

#if defined(_M_X64)     || defined(_M_AMD64) || \
	defined(__x86_64__) || defined(__amd64__)
#define ARCH_X64				1
#define ARCH_64BIT				1
#elif defined(_M_IX86)  || defined(__i386__) || \
	  defined(__i486__) || defined(__i586__) || defined(__i686__)
#define ARCH_X86				1
#define ARCH_32BIT				1
#elif defined(_M_ARM64) || defined(__aarch64__)
#define ARCH_ARM64				1
#define ARCH_64BIT				1
#elif defined(_M_ARM) || defined(__arm__)
#define ARCH_ARM32				1
#define ARCH_32BIT				1
#else
#error "Unsupported architecture."
#endif

#if defined(__cplusplus)
#define LANG_CPP	1
#if defined(COMPILER_MSVC)
#define CPP_VERSION	_MSVC_LANG
#else
#define CPP_VERSION	__cplusplus
#endif

#define CPP11		(CPP_VERSION >= 201103L)
#define CPP14		(CPP_VERSION >= 201402L)
#define CPP17		(CPP_VERSION >= 201703L)
#define CPP20		(CPP_VERSION >= 202002L)
#define CPP23		(CPP_VERSION >= 202302L)
#else
#define LANG_C		1
#if defined(__STDC_VERSION__)
#define C_VERSION	__STDC_VERSION__
#else
#define C_VERSION	0L
#endif
#define C99			(C_VERSION >= 199901L)
#define C11			(C_VERSION >= 201112L)
#define C17			(C_VERSION >= 201710L)
#define C23			(C_VERSION >= 202311L)
#endif

#if defined(LANG_CPP)
#if defined(__has_cpp_attribute)
#define HAS_CPP_ATTR(x)			__has_cpp_attribute(x)
#else
#define HAS_CPP_ATTR(x)			0
#endif
#define HAS_NS_ATTR(ns, name)	HAS_CPP_ATTR(ns::name)
#else
#define HAS_CPP_ATTR(x)			0
#define HAS_NS_ATTR(ns, name)	0
#endif

/**
 * @brief Static assertions.
 */
#if defined(LANG_CPP) && CPP11
#define STATIC_ASSERT(cond, msg)	static_assert(cond, msg)
#elif defined(LANG_C) && C11
#define STATIC_ASSERT(cond, msg)	_Static_assert(cond, msg)
#elif defined(COMPILER_MSVC)
#define STATIC_ASSERT(cond, msg)	typedef char static_assert_##__LINE__[(cond) ? 1 : -1]
#else
#define STATIC_ASSERT(cond, msg)	typedef char static_assert_##__LINE__[(cond) ? 1 : -1]
#endif

/**
 * @brief Inlining before C99.
 */
#if defined(LANG_C) && !C99
#if defined(COMPILER_MSVC)
#define INLINE	__inline
#else
#define INLINE
#endif
#else
#define INLINE	inline
#endif

/**
 * @brief Force function inlining (use sparingly to avoid i-cache pressure).
 */
#if defined(COMPILER_MSVC)
#define FORCE_INLINE			__forceinline
#define ALWAYS_INLINE			__forceinline
#elif defined(COMPILER_CLANG)
#if __has_attribute(always_inline)
#define FORCE_INLINE			__attribute__((always_inline)) inline
#define ALWAYS_INLINE			__attribute__((always_inline)) inline
#else
#define FORCE_INLINE			inline
#define ALWAYS_INLINE			inline
#endif
#elif defined(COMPILER_GCC)
#define FORCE_INLINE			__attribute__((always_inline)) inline
#define ALWAYS_INLINE			__attribute__((always_inline)) inline
#else
#define FORCE_INLINE			inline
#define ALWAYS_INLINE			inline
#endif

/**
 * @brief Prevent function inlining.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, noinline)
#define NO_INLINE				[[gnu::noinline]]
#elif defined(COMPILER_MSVC)
#define NO_INLINE				__declspec(noinline)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define NO_INLINE				__attribute__((noinline))
#else
#define NO_INLINE
#endif

/**
 * @brief Inline all calls within the function's call stack.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, flatten)
#define FLATTEN					[[gnu::flatten]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if __has_attribute(flatten)  || defined(COMPILER_GCC)
#define FLATTEN					__attribute__((flatten))
#else
#define FLATTEN
#endif
#else
#define FLATTEN
#endif

/**
 * @brief Function never returns (e.g., exit(), abort()).
 */
#if defined(LANG_CPP) && CPP11
#define NO_RETURN				[[noreturn]]
#elif defined(LANG_C) && C11
#define NO_RETURN				_Noreturn
#elif defined(COMPILER_MSVC)
#define NO_RETURN				__declspec(noreturn)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define NO_RETURN				__attribute__((noreturn))
#else
#define NO_RETURN
#endif

/**
 * @brief Discard / Ignore.
 *
 * @note Primarily to avoid C-style cast errors in C++
 */
#if defined(LANG_CPP)
#define DISCARD(x)			static_cast<void>(x)
#else
#define DISCARD(x)			((void)(x))
#endif

/**
 * @brief Warn if return value is discarded.
 */
#if defined(LANG_CPP) && CPP17
#define NO_DISCARD				[[nodiscard]]
#define NODISCARD				[[nodiscard]]
#elif defined(LANG_CPP) && HAS_CPP_ATTR(nodiscard)
#define NO_DISCARD				[[nodiscard]]
#define NODISCARD				[[nodiscard]]
#elif defined(LANG_C) && C23
#define NO_DISCARD				[[nodiscard]]
#define NODISCARD				[[nodiscard]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(warn_unused_result)
#define NO_DISCARD				__attribute__((warn_unused_result))
#define NODISCARD				__attribute__((warn_unused_result))
#else
#define NO_DISCARD
#define NODISCARD
#endif
#elif defined(COMPILER_GCC)
#define NO_DISCARD				__attribute__((warn_unused_result))
#define NODISCARD				__attribute__((warn_unused_result))
#elif defined(COMPILER_MSVC)
#define NO_DISCARD				_Check_return_
#define NODISCARD				_Check_return_
#else
#define NO_DISCARD
#define NODISCARD
#endif

/**
 * @brief Suppress unused variable/parameter warnings.
 */
#if defined(LANG_CPP) && CPP17
#define MAYBE_UNUSED			[[maybe_unused]]
#elif defined(LANG_CPP) && HAS_CPP_ATTR(maybe_unused)
#define MAYBE_UNUSED			[[maybe_unused]]
#elif defined(LANG_C) && C23
#define MAYBE_UNUSED			[[maybe_unused]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define MAYBE_UNUSED			__attribute__((unused))
#elif defined(COMPILER_MSVC)
#define MAYBE_UNUSED
#else
#define MAYBE_UNUSED
#endif

/**
 * @brief Mark function or type as deprecated.
 */
#if defined(LANG_CPP) && CPP14
#define DEPRECATED(msg)			[[deprecated(msg)]]
#elif defined(LANG_CPP) && HAS_CPP_ATTR(deprecated)
#define DEPRECATED(msg)			[[deprecated(msg)]]
#elif defined(LANG_C) && C23
#define DEPRECATED(msg)			[[deprecated(msg)]]
#elif defined(COMPILER_MSVC)
#define DEPRECATED(msg)			__declspec(deprecated(msg))
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define DEPRECATED(msg)			__attribute__((deprecated(msg)))
#else
#define DEPRECATED(msg)
#endif

/**
 * @brief Mark intentional switch case fallthrough.
 */
#if defined(LANG_CPP) && CPP17
#define FALLTHROUGH				[[fallthrough]]
#elif defined(LANG_CPP) && HAS_CPP_ATTR(fallthrough)
#define FALLTHROUGH				[[fallthrough]]
#elif defined(LANG_C) && C23
#define FALLTHROUGH				[[fallthrough]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(fallthrough)
#define FALLTHROUGH				__attribute__((fallthrough))
#elif HAS_CPP_ATTR(clang::fallthrough)
#define FALLTHROUGH				[[clang::fallthrough]]
#else
#define FALLTHROUGH				(DISCARD(0))
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 70000)
#define FALLTHROUGH				__attribute__((fallthrough))
#else
#define FALLTHROUGH				(DISCARD(0))
#endif

/**
 * @brief Function has no side effects, may read global memory.
 * Result depends on arguments and global state.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, pure)
#define PURE					[[gnu::pure]]
#define PURE_CALL				[[gnu::pure]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(pure)
#define PURE					__attribute__((pure))
#define PURE_CALL				__attribute__((pure))
#else
#define PURE
#define PURE_CALL
#endif
#elif defined(COMPILER_GCC)
#define PURE					__attribute__((pure))
#define PURE_CALL				__attribute__((pure))
#else
#define PURE
#define PURE_CALL
#endif

/**
 * @brief Function has no side effects, does not read global memory.
 * Result depends only on arguments (stronger than PURE).
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, const)
#define CONST_CALL				[[gnu::const]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(const)
#define CONST_CALL				__attribute__((const))
#else
#define CONST_CALL
#endif
#elif defined(COMPILER_GCC)
#define CONST_CALL				__attribute__((const))
#else
#define CONST_CALL
#endif

/**
 * @brief Function is called frequently.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, hot)
#define HOT_CALL				[[gnu::hot]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(hot)
#define HOT_CALL				__attribute__((hot))
#else
#define HOT_CALL
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 40300)
#define HOT_CALL				__attribute__((hot))
#else
#define HOT_CALL
#endif

/**
 * @brief Function is called rarely.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, cold)
#define COLD_CALL				[[gnu::cold]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(cold)
#define COLD_CALL				__attribute__((cold))
#else
#define COLD_CALL
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 40300)
#define COLD_CALL				__attribute__((cold))
#else
#define COLD_CALL
#endif

/**
 * @brief Function returns non-aliasing pointer to newly allocated memory.
 * @param ... Argument indices for allocation size (1-indexed).
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, malloc)
#define MALLOC_CALL(...)		[[gnu::malloc, gnu::alloc_size(__VA_ARGS__)]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(malloc) && __has_attribute(alloc_size)
#define MALLOC_CALL(...)		__attribute__((malloc, alloc_size(__VA_ARGS__)))
#elif __has_attribute(malloc)
#define MALLOC_CALL(...)		__attribute__((malloc))
#else
#define MALLOC_CALL(...)
#endif
#elif defined(COMPILER_GCC)
#define MALLOC_CALL(...)		__attribute__((malloc, alloc_size(__VA_ARGS__)))
#elif defined(COMPILER_MSVC)
#define MALLOC_CALL(...)		__declspec(restrict)
#else
#define MALLOC_CALL(...)
#endif

/**
 * @brief Return pointer points to memory defined by argument
 *
 * @param ... The index of the argument defining the size of the memory returned (1-indexing)
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if __has_attribute(alloc_size) || defined(COMPILER_GCC)
#define ALLOC_SIZE(...)	__attribute__((alloc_size(__VA_ARGS__)))
#else
#define ALLOC_SIZE(...)
#endif
#if __has_attribute(alloc_align) || defined(COMPILER_GCC)
#define ALLOC_ALIGN(n)	__attribute__((alloc_align(n)))
#else
#define ALLOC_ALIGN(n)
#endif
#else
#define ALLOC_SIZE(...)
#define ALLOC_ALIGN(n)
#endif

/**
 * @brief Mark symbol as exported (visible in shared library).
 */
#if defined(PLATFORM_WINDOWS)
#if defined(COMPILER_MSVC)
#define API_CALL				__declspec(dllexport)
#define API_IMPORT				__declspec(dllimport)
#define API_LOCAL
#else
#define API_CALL				__attribute__((dllexport))
#define API_IMPORT				__attribute__((dllimport))
#define API_LOCAL
#endif
#else
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, visibility)
#define API_CALL				[[gnu::visibility("default")]]
#define API_IMPORT				[[gnu::visibility("default")]]
#define API_LOCAL				[[gnu::visibility("hidden")]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define API_CALL				__attribute__((visibility("default")))
#define API_IMPORT				__attribute__((visibility("default")))
#define API_LOCAL				__attribute__((visibility("hidden")))
#else
#define API_CALL
#define API_IMPORT
#define API_LOCAL
#endif
#endif

/**
 * @brief Hint that condition is likely true.
 *
 * @warn Syntax changes between builtin and attribute versions
 */
#if defined(LANG_CPP) && (CPP20 || HAS_CPP_ATTR(likely))
#define LIKELY			[[likely]]
#elif defined(LANG_C) && C23
#define LIKELY			[[likely]]
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#if __has_builtin(__builtin_expect)
#define LIKELY(cond)	__builtin_expect(!!(cond), 1)
#else
#define LIKELY(cond)	(!!(cond))
#endif
#else
#define LIKELY(cond)	(!!(cond))
#endif

/**
 * @brief Hint that condition is unlikely true.
 *
 * @warn Syntax changes between builtin and attribute versions
 */
#if defined(LANG_CPP) && (CPP20 || HAS_CPP_ATTR(unlikely))
#define UNLIKELY		[[unlikely]]
#elif defined(LANG_C) && C23
#define UNLIKELY		[[unlikely]]
#elif defined(COMPILER_GCC) || defined(COMPILER_CLANG)
#if __has_builtin(__builtin_expect)
#define UNLIKELY(cond)	__builtin_expect(!!(cond), 0)
#else
#define UNLIKELY(cond)	(!!(cond))
#endif
#else
#define UNLIKELY(cond)	(!!(cond))
#endif

/**
 * @brief Branch prediction with explicit condition (evaluates to condition).
 */
#if defined(COMPILER_CLANG)
#if __has_builtin(__builtin_expect)
#define EXPECT_TRUE(x)			__builtin_expect(!!(x), 1)
#define EXPECT_FALSE(x)			__builtin_expect(!!(x), 0)
#else
#define EXPECT_TRUE(x)			(!!(x))
#define EXPECT_FALSE(x)			(!!(x))
#endif
#elif defined(COMPILER_GCC)
#define EXPECT_TRUE(x)			__builtin_expect(!!(x), 1)
#define EXPECT_FALSE(x)			__builtin_expect(!!(x), 0)
#else
#define EXPECT_TRUE(x)			(!!(x))
#define EXPECT_FALSE(x)			(!!(x))
#endif

/**
 * @brief Branch prediction with explicit probability (0.0 to 1.0).
 */
#if defined(COMPILER_CLANG)
#if __has_builtin(__builtin_expect_with_probability)
#define COND_PROB(x, p)			__builtin_expect_with_probability(!!(x), 1, (p))
#elif __has_builtin(__builtin_expect)
#define COND_PROB(x, p)			__builtin_expect(!!(x), (p) >= 0.5)
#else
#define COND_PROB(x, p)			(!!(x))
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 90000)
#define COND_PROB(x, p)			__builtin_expect_with_probability(!!(x), 1, (p))
#elif defined(COMPILER_GCC)
#define COND_PROB(x, p)			__builtin_expect(!!(x), (p) >= 0.5)
#else
#define COND_PROB(x, p)			(!!(x))
#endif

/**
 * @brief Tell compiler to assume condition is true (enables optimizations).
 * @warning Undefined behavior if condition is false at runtime.
 */
#if defined(LANG_CPP) && CPP23
#define ASSUME(x)				[[assume(!!(x))]]
#elif defined(LANG_CPP) && HAS_CPP_ATTR(assume)
#define ASSUME(x)				[[assume(!!(x))]]
#elif defined(COMPILER_MSVC)
#define ASSUME(x)				__assume(!!(x))
#elif defined(COMPILER_CLANG)
#if __has_builtin(__builtin_assume)
#define ASSUME(x)				__builtin_assume(!!(x))
#else
#define ASSUME(x)				(DISCARD(0))
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 130000)
#define ASSUME(x)				__attribute__((assume(!!(x))))
#else
#define ASSUME(x)				(DISCARD(0))
#endif

/**
 * @brief Mark code path as unreachable (enables dead code elimination).
 * @warning Undefined behavior if reached at runtime.
 */
#if defined(COMPILER_MSVC)
#define UNREACHABLE				__assume(0)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define UNREACHABLE				__builtin_unreachable()
#else
#define UNREACHABLE				(DISCARD(0))
#endif

/**
 * @brief Align variable or type to n-byte boundary.
 */
#if defined(LANG_CPP) && CPP11
#define ALIGNED(n)				alignas(n)
#define ALIGN(n)				alignas(n)
#elif defined(LANG_C) && C11
#define ALIGNED(n)				_Alignas(n)
#define ALIGN(n)				_Alignas(n)
#elif defined(COMPILER_MSVC)
#define ALIGNED(n)				__declspec(align(n))
#define ALIGN(n)				__declspec(align(n))
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define ALIGNED(n)				__attribute__((aligned(n)))
#define ALIGN(n)				__attribute__((aligned(n)))
#else
#define ALIGNED(n)
#define ALIGN(n)
#endif

/**
 * @brief Pack structure with no padding.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, packed)
#define PACKED					[[gnu::packed]]
#elif defined(COMPILER_MSVC)
#define PACKED					/* Use #pragma pack instead */
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define PACKED					__attribute__((packed))
#else
#define PACKED
#endif

/**
 * @brief Allow empty base optimization (C++20).
 */
#if defined(LANG_CPP) && CPP20 && !defined(COMPILER_MSVC)
#define NO_UNIQUE_ADDRESS		[[no_unique_address]]
#elif defined(COMPILER_MSVC) && (COMPILER_VERSION >= 1929)
#define NO_UNIQUE_ADDRESS		[[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS
#endif

/**
 * @brief Pointer does not alias other pointers (enables optimizations).
 */
#if defined(LANG_C) && C99
#define RESTRICT				restrict
#else
#if defined(COMPILER_MSVC)
#define RESTRICT				__restrict
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define RESTRICT				__restrict__
#else
#define RESTRICT
#endif
#endif

/**
 * @brief All pointer arguments must not be null.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, nonnull)
#define NO_NULL_ARGS			[[gnu::nonnull]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define NO_NULL_ARGS			__attribute__((nonnull))
#else
#define NO_NULL_ARGS
#endif

/**
 * @brief Specific arguments must not be null (1-indexed).
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, nonnull)
#define NON_NULL_ARGS(...)		[[gnu::nonnull(__VA_ARGS__)]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define NON_NULL_ARGS(...)		__attribute__((nonnull(__VA_ARGS__)))
#else
#define NON_NULL_ARGS(...)
#endif

/**
 * @brief Function never returns null.
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, returns_nonnull)
#define RETURNS_NON_NULL		[[gnu::returns_nonnull]]
#elif defined(COMPILER_CLANG)
#if __has_attribute(returns_nonnull)
#define RETURNS_NON_NULL		__attribute__((returns_nonnull))
#else
#define RETURNS_NON_NULL
#endif
#elif defined(COMPILER_GCC)
#define RETURNS_NON_NULL		__attribute__((returns_nonnull))
#else
#define RETURNS_NON_NULL
#endif

/**
 * @brief Thread-local variable.
 */
#if defined(LANG_CPP) && CPP11
#define THREAD_LOCAL			thread_local
#elif defined(LANG_C) && C11
#define THREAD_LOCAL			_Thread_local
#elif defined(COMPILER_MSVC)
#define THREAD_LOCAL			__declspec(thread)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define THREAD_LOCAL			__thread
#else
#define THREAD_LOCAL
#endif

/**
 * @brief TLS access model for shared libraries (faster but more restrictive).
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define TLS_MODEL				__attribute__((tls_model("initial-exec")))
#else
#define TLS_MODEL
#endif

/**
 * @brief Clang-specific thread-safety annotations.
 */
#if defined(COMPILER_CLANG) && __has_attribute(capability)
#define CAPABILITY(x)			__attribute__((capability(x)))
#define GUARDED_BY(x)			__attribute__((guarded_by(x)))
#define ACQUIRE_CAPABILITY(...)	__attribute__((acquire_capability(__VA_ARGS__)))
#define RELEASE_CAPABILITY(...)	__attribute__((release_capability(__VA_ARGS__)))
#else
#define CAPABILITY(x)
#define GUARDED_BY(x)
#define ACQUIRE_CAPABILITY(...)
#define RELEASE_CAPABILITY(...)
#endif

/**
 * @brief Helpful to enforce API rules with custom messages (contract-like).
 */
#if defined(COMPILER_CLANG) && __has_attribute(diagnose_if)
#define CONTRACT(cond, msg, type)	__attribute__((diagnose_if(cond, msg, type)))
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 80000)
#define CONTRACT(cond, msg, type)	__attribute__((diagnose_if(cond, msg, type)))
#else
#define CONTRACT(cond, msg, type)
#endif

/**
 * @brief Compiler memory barrier (prevents reordering by compiler only).
 */
#if defined(COMPILER_MSVC)
#define COMPILER_BARRIER()		_ReadWriteBarrier()
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define COMPILER_BARRIER()		__asm__ __volatile__("" ::: "memory")
#else
#define COMPILER_BARRIER()		(DISCARD(0))
#endif

/**
 * @brief Full memory barrier (prevents reordering by compiler and CPU).
 */
#if defined(COMPILER_MSVC)
#if defined(ARCH_ARM64)
#define MEMORY_BARRIER()		__dmb(_ARM64_BARRIER_SY)
#else
#define MEMORY_BARRIER()		_mm_mfence()
#endif
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if defined(ARCH_ARM64)
#define MEMORY_BARRIER()		__asm__ __volatile__("dmb sy" ::: "memory")
#elif defined(ARCH_X64) || defined(ARCH_X86)
#define MEMORY_BARRIER()		__asm__ __volatile__("mfence" ::: "memory")
#else
#define MEMORY_BARRIER()		__sync_synchronize()
#endif
#else
#define MEMORY_BARRIER()		(DISCARD(0))
#endif

/**
 * @brief Acquire barrier (loads after barrier see effects of prior stores).
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define LOAD_BARRIER()			__atomic_thread_fence(__ATOMIC_ACQUIRE)
#elif defined(COMPILER_MSVC)
#define LOAD_BARRIER()			_ReadBarrier()
#else
#define LOAD_BARRIER()			MEMORY_BARRIER()
#endif

/**
 * @brief Release barrier (stores before barrier visible to later loads).
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define STORE_BARRIER()			__atomic_thread_fence(__ATOMIC_RELEASE)
#elif defined(COMPILER_MSVC)
#define STORE_BARRIER()			_WriteBarrier()
#else
#define STORE_BARRIER()			MEMORY_BARRIER()
#endif

#if defined(COMPILER_MSVC)
#define CDECL					__cdecl
#define STDCALL					__stdcall
#define FASTCALL				__fastcall
#define THISCALL				__thiscall
#define VECTORCALL				__vectorcall
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if defined(ARCH_X86)
#define CDECL					__attribute__((cdecl))
#define STDCALL					__attribute__((stdcall))
#define FASTCALL				__attribute__((fastcall))
#define THISCALL				__attribute__((thiscall))
#else
#define CDECL
#define STDCALL
#define FASTCALL
#define THISCALL
#endif
#if defined(COMPILER_CLANG)
#define VECTORCALL				__attribute__((vectorcall))
#else
#define VECTORCALL
#endif
#else
#define CDECL
#define STDCALL
#define FASTCALL
#define THISCALL
#define VECTORCALL
#endif

/**
 * @brief Disable stack protection for function.
 */
#if defined(COMPILER_MSVC)
#define NO_STACK_PROTECT		__declspec(safebuffers)
#elif defined(COMPILER_CLANG)
#if __has_attribute(no_stack_protector)
#define NO_STACK_PROTECT		__attribute__((no_stack_protector))
#else
#define NO_STACK_PROTECT
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 40900)
#define NO_STACK_PROTECT		__attribute__((no_stack_protector))
#else
#define NO_STACK_PROTECT
#endif

/**
 * @brief Check for SSE2 and AVX2.
 */
#if defined(ARCH_X64) || defined(ARCH_X86)
#if defined(__SSE2__) || (defined(COMPILER_MSVC) && \
   (defined(ARCH_X64) || (_M_IX86_FP >= 2)))
#define HAS_SSE2	1
#endif
#if defined(__AVX2__)
#define HAS_AVX2	1
#endif
#endif

/**
 * @brief Check for ARM neon.
 */
#if defined(ARCH_ARM64) || defined(ARCH_ARM32)
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#define HAS_NEON	1
#endif
#endif

/**
 * @brief Disable Control Flow Integrity checks for function.
 */
#if defined(COMPILER_MSVC)
#define NO_CFI					__declspec(guard(nocf))
#elif defined(COMPILER_CLANG)
#if __has_feature(cfi)
#define NO_CFI					__attribute__((no_sanitize("cfi")))
#elif __has_attribute(nocf_check)
#define NO_CFI					__attribute__((nocf_check))
#else
#define NO_CFI
#endif
#elif defined(COMPILER_GCC) && (COMPILER_VERSION >= 90000)
#define NO_CFI					__attribute__((nocf_check))
#else
#define NO_CFI
#endif

/**
 * @brief Disable sanitation policies.
 */
#if defined(COMPILER_CLANG)
#if __has_attribute(no_sanitize)
#define NO_SANITIZE(kind)		__attribute__((no_sanitize(kind)))
#else
#define NO_SANITIZE(kind)
#endif
#if __has_attribute(no_sanitize_address)
#define NO_ASAN					__attribute__((no_sanitize_address))
#else
#define NO_ASAN
#endif
#if __has_attribute(no_sanitize_thread)
#define NO_TSAN					__attribute__((no_sanitize_thread))
#else
#define NO_TSAN
#endif
#if __has_attribute(no_sanitize_undefined)
#define NO_UBSAN				__attribute__((no_sanitize_undefined))
#else
#define NO_UBSAN
#endif
#elif defined(COMPILER_GCC)
#if __has_attribute(no_sanitize)
#define NO_SANITIZE(kind)		__attribute__((no_sanitize(kind)))
#else
#define NO_SANITIZE(kind)
#endif
#define NO_ASAN
#define NO_TSAN
#define NO_UBSAN
#elif defined(COMPILER_MSVC)
#define NO_SANITIZE(kind)
#define NO_ASAN
#define NO_TSAN
#define NO_UBSAN
#else
#define NO_SANITIZE(kind)
#define NO_ASAN
#define NO_TSAN
#define NO_UBSAN
#endif

/**
 * @brief Function called before main() (or on library load).
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, constructor)
#define CONSTRUCTOR				[[gnu::constructor]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define CONSTRUCTOR				__attribute__((constructor))
#elif defined(COMPILER_MSVC)
#define CONSTRUCTOR
#else
#define CONSTRUCTOR
#endif

/**
 * @brief Function called after main() returns (or on library unload).
 */
#if defined(LANG_CPP) && HAS_NS_ATTR(gnu, destructor)
#define DESTRUCTOR				[[gnu::destructor]]
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define DESTRUCTOR				__attribute__((destructor))
#elif defined(COMPILER_MSVC)
#define DESTRUCTOR
#else
#define DESTRUCTOR
#endif

/**
 * @brief Prefetch data into cache.
 * @param addr Address to prefetch.
 * @param rw 0=read, 1=write
 * @param locality 0=no temporal locality, 3=high temporal locality
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define PREFETCH(addr, rw, locality)	__builtin_prefetch((addr), (rw), (locality))
#define PREFETCH_READ(addr)				__builtin_prefetch((addr), 0, 3)
#define PREFETCH_WRITE(addr)			__builtin_prefetch((addr), 1, 3)
#elif defined(COMPILER_MSVC) && (defined(ARCH_X64) || defined(ARCH_X86))
#include <xmmintrin.h>
#define PREFETCH(addr, rw, locality)	_mm_prefetch((const char*)(addr), _MM_HINT_T0)
#define PREFETCH_READ(addr)				_mm_prefetch((const char*)(addr), _MM_HINT_T0)
#define PREFETCH_WRITE(addr)			_mm_prefetch((const char*)(addr), _MM_HINT_T0)
#else
#define PREFETCH(addr, rw, locality)	(DISCARD(0))
#define PREFETCH_READ(addr)				(DISCARD(0))
#define PREFETCH_WRITE(addr)			(DISCARD(0))
#endif

/* Temporal locality hints for PREFETCH */
#define PREFETCH_LOCALITY_NONE			0
#define PREFETCH_LOCALITY_LOW			1
#define PREFETCH_LOCALITY_MEDIUM		2
#define PREFETCH_LOCALITY_HIGH			3

/**
 * @brief Get number of elements in array.
 */
#define ARRAY_SIZE(arr)	(sizeof(arr) / sizeof((arr)[0]))

/**
 * @brief Get pointer to end of array.
 */
#define ARRAY_END(arr)	((arr) + ARRAY_SIZE(arr))

/**
 * @brief Cast expression to void to suppress unused warnings.
 */
#if defined(COMPILER_CLANG)
#define UNUSED_VAR(...)											\
	do															\
	{															\
		_Pragma("clang diagnostic push")						\
		_Pragma("clang diagnostic ignored \"-Wunused-value\"")	\
		DISCARD(((__VA_ARGS__), 0));							\
		_Pragma("clang diagnostic pop")							\
	}	while (0)
#elif defined(COMPILER_GCC)
#define UNUSED_VAR(...)											\
	do															\
	{															\
		_Pragma("GCC diagnostic push")							\
		_Pragma("GCC diagnostic ignored \"-Wunused-value\"")	\
		DISCARD(((__VA_ARGS__), 0));							\
		_Pragma("GCC diagnostic pop")							\
	}	while (0)
#else
#define UNUSED_VAR(...)	(DISCARD(((__VA_ARGS__), 0)))
#endif

/**
 * @brief Assume a pointer is aligned in regards to `a`.
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define ASSUME_ALIGNED(ptr, a)	((__typeof__(ptr))__builtin_assume_aligned((ptr), (a)))
#else
#define ASSUME_ALIGNED(ptr, a)	(ptr)
#endif

/**
 * @brief Get byte offset of member in struct.
 */
#if defined(LANG_CPP)
#include <cstddef>
#define OFFSET_OF(type, member)	offsetof(type, member)
#else
#include <stddef.h>
#define OFFSET_OF(type, member)	offsetof(type, member)
#endif

/**
 * @brief Get pointer to struct from pointer to member.
 */
#define CONTAINER_OF(ptr, type, member) \
	((type*)((char*)(ptr) - OFFSET_OF(type, member)))

/**
 * @brief Get minimum of two values.
 */
#define MIN(a, b)				(((a) < (b)) ? (a) : (b))

/**
 * @brief Get maximum of two values.
 */
#define MAX(a, b)				(((a) > (b)) ? (a) : (b))

/**
 * @brief Clamp value between min and max.
 */
#define CLAMP(x, lo, hi)		(((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

/**
 * @brief Check if value is power of 2.
 */
#define IS_POWER_OF_2(x)		((x) && !((x) & ((x) - 1)))

/**
 * @brief Round up to next multiple of alignment (power of 2 only).
 */
#define ALIGN_UP(val, align)	(((val) + (align) - 1) & ~((align) - 1))

/**
 * @brief Round down to previous multiple of alignment (power of 2 only).
 */
#define ALIGN_DOWN(val, align)	((val) & ~((align) - 1))

/**
 * @brief Check if value is aligned to boundary.
 */
#define IS_ALIGNED(val, align)	(!((val) & ((align) - 1)))

/**
 * @brief Create bit mask for bit n.
 */
#define BIT(n)					(1ULL << (n))

/**
 * @brief Set bit n in value.
 */
#define BIT_SET(val, n)			((val) | BIT(n))

/**
 * @brief Clear bit n in value.
 */
#define BIT_CLEAR(val, n)		((val) & ~BIT(n))

/**
 * @brief Toggle bit n in value.
 */
#define BIT_TOGGLE(val, n)		((val) ^ BIT(n))

/**
 * @brief Check if bit n is set.
 */
#define BIT_CHECK(val, n)		(!!((val) & BIT(n)))

#define STRINGIFY_(x)			#x
#define STRINGIFY(x)			STRINGIFY_(x)

#define CONCAT_(a, b)			a##b
#define CONCAT(a, b)			CONCAT_(a, b)

/**
 * @brief Trigger debugger breakpoint.
 */
#if defined(COMPILER_MSVC)
#define DEBUG_BREAK()			__debugbreak()
#elif defined(COMPILER_CLANG)
#if __has_builtin(__builtin_debugtrap)
#define DEBUG_BREAK()			__builtin_debugtrap()
#else
#define DEBUG_BREAK()			__asm__ __volatile__("int3")
#endif
#elif defined(COMPILER_GCC)
#if defined(ARCH_X64) || defined(ARCH_X86)
#define DEBUG_BREAK()			__asm__ __volatile__("int3")
#elif defined(ARCH_ARM64)
#define DEBUG_BREAK()			__asm__ __volatile__("brk #0")
#elif defined(ARCH_ARM32)
#define DEBUG_BREAK()			__asm__ __volatile__("bkpt #0")
#else
#include <signal.h>
#define DEBUG_BREAK()			raise(SIGTRAP)
#endif
#else
#define DEBUG_BREAK()			(DISCARD(0))
#endif


/**
 * @brief CPU pause.
 */
#if defined(COMPILER_MSVC) && (defined(ARCH_X64) || defined(ARCH_X86))
#include <immintrin.h>
#define CPU_RELAX()		_mm_pause()
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if defined(ARCH_X64) || defined(ARCH_X86)
#define CPU_RELAX()		__asm__ __volatile__("pause")
#elif defined(ARCH_ARM64) || defined(ARCH_ARM32)
#define CPU_RELAX()		__asm__ __volatile__("yield")
#else
#define CPU_RELAX()		COMPILER_BARRIER()
#endif
#else
#define CPU_RELAX()		COMPILER_BARRIER()
#endif

/**
 * @brief Runtime assertion.
 */
#if defined(LANG_CPP)
#include <cassert>
#else
#include <assert.h>
#endif
#define ASSERT(cond, msg)		assert((cond) && (msg))

/**
 * @brief Guarantee condition and allow compiler to optimize based on it.
 * @warning Undefined behavior if condition is false at runtime.
 */
#define GUARANTEE(cond, msg)	\
	do							\
	{							\
		ASSERT((cond), msg);	\
		ASSUME(cond);			\
	}	while (0)

#if defined(LANG_CPP)
#include <cstring>
#include <cstdint>
#else
#include <string.h>
#include <stdint.h>
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if __has_builtin(__builtin_memcpy_inline)
#define memcpy_constexpr(dst, src, n)	__builtin_memcpy_inline((dst), (src), (n))
#else
#if defined(LANG_CPP)
#define memcpy_constexpr(dst, src, n)	std::memcpy((dst), (src), (n))
#else
#define memcpy_constexpr(dst, src, n)	memcpy((dst), (src), (n))
#endif
#endif
#if __has_builtin(__builtin_memset_inline)
#define memset_constexpr(dst, val, n)	__builtin_memset_inline((dst), (val), (n))
#else
#if defined(LANG_CPP)
#define memset_constexpr(dst, val, n)	std::memset((dst), (val), (n))
#else
#define memset_constexpr(dst, val, n)	memset((dst), (val), (n))
#endif
#endif
#else
#if defined(LANG_CPP)
#define memcpy_constexpr(dst, src, n)	std::memcpy((dst), (src), (n))
#define memset_constexpr(dst, val, n)	std::memset((dst), (val), (n))
#else
#define memcpy_constexpr(dst, src, n)	memcpy((dst), (src), (n))
#define memset_constexpr(dst, val, n)	memset((dst), (val), (n))
#endif
#endif

#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
/**
 * @brief Create weak symbol alias.
 */
#define WEAK_ALIAS(old, new) \
	extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

/**
 * @brief Create strong symbol alias.
 */
#define STRONG_ALIAS(old, new) \
	extern __typeof(old) new __attribute__((__alias__(#old)))
#else
#define WEAK_ALIAS(old, new)
#define STRONG_ALIAS(old, new)
#endif

#if defined(LANG_C) && C11
/**
 * @brief Check if two expressions have compatible types.
 */
#define IS_SAME_TYPE(a, b)		__builtin_types_compatible_p(typeof(a), typeof(b))
#elif defined(LANG_CPP)
#include <type_traits>
#define IS_SAME_TYPE(a, b)		std::is_same<decltype(a), decltype(b)>::value
#else
#define IS_SAME_TYPE(a, b)		1
#endif

/**
 * @brief Offset pointer by byte count.
 */
#define OFFSET_PTR(ptr, off)	((void*)((unsigned char*)(ptr) + (ptrdiff_t)(off)))

/**
 * @brief Get byte difference between two pointers.
 */
#define PTR_DIFF(a, b)			((ptrdiff_t)((const unsigned char*)(a) - (const unsigned char*)(b)))

#if defined(__BYTE_ORDER__)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ENDIAN_LITTLE			1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ENDIAN_BIG				1
#endif
#elif defined(COMPILER_MSVC)
#define ENDIAN_LITTLE			1
#elif defined(ARCH_X86) || defined(ARCH_X64) || defined(ARCH_ARM64)
#define ENDIAN_LITTLE			1
#else
#define ENDIAN_LITTLE			1
#endif

/**
 * @brief Rotates.
 */
#if defined(COMPILER_MSVC)
#include <intrin.h>
#define ROTL32(x, r)		_rotl((x), (r))
#define ROTR32(x, r)		_rotr((x), (r))
#if defined(ARCH_X64)
#define ROTL64(x, r)		_rotl64((x), (r))
#define ROTR64(x, r)		_rotr64((x), (r))
#else
#define ROTL64(x, r)		(((uint64_t)(x) << (r)) | ((uint64_t)(x) >> (64 - (r))))
#define ROTR64(x, r)		(((uint64_t)(x) >> (r)) | ((uint64_t)(x) << (64 - (r))))
#endif
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#if __has_builtin(__builtin_rotateleft32)
#define ROTL32(x, r)		__builtin_rotateleft32((x), (r))
#define ROTR32(x, r)		__builtin_rotateright32((x), (r))
#else
#define ROTL32(x, r)		(((uint32_t)(x) << (r)) | ((uint32_t)(x) >> (32 - (r))))
#define ROTR32(x, r)		(((uint32_t)(x) >> (r)) | ((uint32_t)(x) << (32 - (r))))
#endif
#if __has_builtin(__builtin_rotateleft64)
#define ROTL64(x, r)		__builtin_rotateleft64((x), (r))
#define ROTR64(x, r)		__builtin_rotateright64((x), (r))
#else
#define ROTL64(x, r)		(((uint64_t)(x) << (r)) | ((uint64_t)(x) >> (64 - (r))))
#define ROTR64(x, r)		(((uint64_t)(x) >> (r)) | ((uint64_t)(x) << (64 - (r))))
#endif
#else
#define ROTL32(x, r)		(((uint32_t)(x) << (r)) | ((uint32_t)(x) >> (32 - (r))))
#define ROTR32(x, r)		(((uint32_t)(x) >> (r)) | ((uint32_t)(x) << (32 - (r))))
#define ROTL64(x, r)		(((uint64_t)(x) << (r)) | ((uint64_t)(x) >> (64 - (r))))
#define ROTR64(x, r)		(((uint64_t)(x) >> (r)) | ((uint64_t)(x) << (64 - (r))))
#endif

/**
 * @brief Byte swap.
 */
#if defined(COMPILER_MSVC)
#include <stdlib.h>
#define BSWAP16(x)	_byteswap_ushort(x)
#define BSWAP32(x)	_byteswap_ulong(x)
#define BSWAP64(x)	_byteswap_uint64(x)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define BSWAP16(x)	__builtin_bswap16(x)
#define BSWAP32(x)	__builtin_bswap32(x)
#define BSWAP64(x)	__builtin_bswap64(x)
#else
#define BSWAP16(x)	((uint16_t)(((x) >> 8) | ((x) << 8)))
#define BSWAP32(x)	((uint32_t)(((x) >> 24) | (((x) >> 8) & 0xFF00) | \
					(((x) << 8) & 0xFF0000) | ((x) << 24)))
#define BSWAP64(x)	((uint64_t)BSWAP32((uint32_t)(x)) << 32 | BSWAP32((uint32_t)((x) >> 32)))
#endif

/**
 * @brief Count leading zeros.
 */
#if defined(COMPILER_MSVC)
#include <intrin.h>
static FORCE_INLINE
unsigned int clz32(uint32_t x)
{
	unsigned long idx;
	return _BitScanReverse(&idx, x) ? (31 - idx) : 32;
}
static FORCE_INLINE
unsigned int clz64(uint64_t x)
{
	unsigned long idx;
#if defined(ARCH_X64)
	return _BitScanReverse64(&idx, x) ? (63 - idx) : 64;
#else
	if (_BitScanReverse(&idx, (uint32_t)(x >> 32)))
		return 31 - idx;
	return _BitScanReverse(&idx, (uint32_t)x) ? (63 - idx) : 64;
#endif
}
#define CLZ32(x)	clz32(x)
#define CLZ64(x)	clz64(x)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define CLZ32(x)	((x) ? (unsigned int)__builtin_clz(x)   : 32u)
#define CLZ64(x)	((x) ? (unsigned int)__builtin_clzll(x) : 64u)
#else
static FORCE_INLINE
unsigned int clz32_fallback(uint32_t x)
{
	unsigned int n = 32;
	if (x >> 16) { n -= 16; x >>= 16; }
	if (x >> 8)  { n -= 8;  x >>= 8;  }
	if (x >> 4)  { n -= 4;  x >>= 4;  }
	if (x >> 2)  { n -= 2;  x >>= 2;  }
	if (x >> 1)  { n -= 1;  x >>= 1;  }
	return n - x;
}
#define CLZ32(x)	clz32_fallback(x)
#define CLZ64(x)	((uint32_t)((x) >> 32) ? CLZ32((uint32_t)((x) >> 32)) \
										   : (32 + CLZ32((uint32_t)(x))))
#endif

/**
 * @brief Count trailing zeros.
 */
#if defined(COMPILER_MSVC)
static FORCE_INLINE
unsigned int ctz32(uint32_t x)
{
	unsigned long idx;
	return _BitScanForward(&idx, x) ? idx : 32;
}
static FORCE_INLINE
unsigned int ctz64(uint64_t x)
{
	unsigned long idx;
#if defined(ARCH_X64)
	return _BitScanForward64(&idx, x) ? idx : 64;
#else
	if (_BitScanForward(&idx, (uint32_t)x))
		return idx;
	return _BitScanForward(&idx, (uint32_t)(x >> 32)) ? (32 + idx) : 64;
#endif
}
#define CTZ32(x)	ctz32(x)
#define CTZ64(x)	ctz64(x)
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define CTZ32(x)	((x) ? (unsigned int)__builtin_ctz(x)   : 32u)
#define CTZ64(x)	((x) ? (unsigned int)__builtin_ctzll(x) : 64u)
#else
#define CTZ32(x)	((x) ? (31u - CLZ32((x) & -(x))) : 32u)
#define CTZ64(x)	((x) ? (63u - CLZ64((x) & -(x))) : 64u)
#endif

/**
 * @brief Population count (count set bits).
 */
#if defined(COMPILER_MSVC)
#include <intrin.h>
#define POPCOUNT32(x)	__popcnt(x)
#if defined(ARCH_X64)
#define POPCOUNT64(x)	__popcnt64(x)
#else
#define POPCOUNT64(x)	(__popcnt((uint32_t)(x)) + __popcnt((uint32_t)((x) >> 32)))
#endif
#elif defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define POPCOUNT32(x)	((unsigned int)__builtin_popcount(x))
#define POPCOUNT64(x)	((unsigned int)__builtin_popcountll(x))
#else
static FORCE_INLINE
unsigned int popcount32_fallback(uint32_t x)
{
	x =  x - ((x >> 1) & 0x55555555u);
	x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
	x = (x + (x >> 4)) & 0x0F0F0F0Fu;
	return (x * 0x01010101u) >> 24;
}
#define POPCOUNT32(x)	popcount32_fallback(x)
#define POPCOUNT64(x)	(POPCOUNT32((uint32_t)(x)) + POPCOUNT32((uint32_t)((x) >> 32)))
#endif

/**
 * @brief Mark function as printf-like for format checking.
 * @param fmt_idx Index of format string argument (1-indexed).
 * @param va_idx Index of first variadic argument (1-indexed).
 */
#if defined(COMPILER_CLANG) || defined(COMPILER_GCC)
#define FORMAT_PRINTF(fmt_idx, va_idx)	__attribute__((format(printf, fmt_idx, va_idx)))
#define FORMAT_SCANF(fmt_idx, va_idx)	__attribute__((format(scanf, fmt_idx, va_idx)))
#elif defined(COMPILER_MSVC)
#include <sal.h>
#define FORMAT_PRINTF(fmt_idx, va_idx)	_Printf_format_string_
#define FORMAT_SCANF(fmt_idx, va_idx)	_Scanf_format_string_
#else
#define FORMAT_PRINTF(fmt_idx, va_idx)
#define FORMAT_SCANF(fmt_idx, va_idx)
#endif

#endif /* __ANNOTATIONS_H_ */
