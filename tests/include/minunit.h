/* Based on minunit. http://www.jera.com/techinfo/jtns/jtn002.html */

#include <stdio.h>
#include <stdlib.h>

extern int tests_run;
extern int tests_failed;

#define mu_assert(test, message) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { \
		char* message = test(); \
		++tests_run; \
		tests_failed += (message != (void*)0); \
		if(message) \
			fprintf(stderr, "[%d]: %s failed: %s\n", tests_run, #test, message); \
		else \
			printf("[%d]: %s passed\n", tests_run, #test); \
		} while (0)