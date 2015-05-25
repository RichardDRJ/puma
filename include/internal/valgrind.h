#ifndef _PUMA__VALGRIND_H_
#define _PUMA__VALGRIND_H_

#if defined(NDEBUG) || defined(NOVALGRIND)
#define VALGRIND_MAKE_MEM_DEFINED(addr, size) (void)(addr); (void)(size)
#define VALGRIND_MAKE_MEM_NOACCESS(addr, size) (void)(addr); (void)(size)
#define VALGRIND_MAKE_MEM_UNDEFINED(addr, size) (void)(addr); (void)(size)
#define VALGRIND_MALLOCLIKE_BLOCK(addr, size, rSize, zero) (void)(addr);		\
		(void)(size); (void)(rSize); (void)(zero)
#define VALGRIND_FREELIKE_BLOCK(addr, rSize) (void)(addr); (void)(rSize)
#else
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

#endif // _PUMA__VALGRIND_H_
