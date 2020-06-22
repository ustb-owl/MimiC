#ifndef XSTL_OS_H_
#define XSTL_OS_H_

// some macros about current operating system

#if defined(_WIN64) || defined(__MINGW64__) || \
    (defined(__CYGWIN__) && defined(__x86_64__))
#define XSTL_OS_64_WIN
#elif defined(__x86_64__)
#define XSTL_OS_64_GCC
#endif

#if !defined(XSTL_OS_64) && !defined(XSTL_OS_32)
#if defined(XSTL_OS_64_GCC) || defined(XSTL_OS_64_WIN)
#define XSTL_OS_64
#else
#define XSTL_OS_32
#endif
#endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_MSC_VER)
#define XSTL_OS_WINDOWS
#ifdef _MSC_VER
#define XSTL_OS_WIN_VS
#endif
#endif

#ifdef __APPLE__
#define XSTL_OS_MACOS
#endif

#ifdef __linux__
#define XSTL_OS_LINUX
#endif

#endif  // XSTL_OS_H_
