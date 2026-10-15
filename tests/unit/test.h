#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <string.h>

extern int g_tests_run;
extern int g_tests_failed;

#define CHECK(cond)                                                                                \
	do {                                                                                           \
		g_tests_run++;                                                                             \
		if (!(cond)) {                                                                             \
			g_tests_failed++;                                                                      \
			printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                               \
		}                                                                                          \
	} while (0)

#define CHECK_INT(actual, expected)                                                                \
	do {                                                                                           \
		long a_ = (long)(actual);                                                                  \
		long e_ = (long)(expected);                                                                \
		g_tests_run++;                                                                             \
		if (a_ != e_) {                                                                            \
			g_tests_failed++;                                                                      \
			printf("  FAIL %s:%d: %s is %ld, expected %ld\n", __FILE__, __LINE__, #actual, a_,     \
			       e_);                                                                            \
		}                                                                                          \
	} while (0)

#define CHECK_STR(actual, expected)                                                                \
	do {                                                                                           \
		const char *a_ = (actual);                                                                 \
		const char *e_ = (expected);                                                               \
		g_tests_run++;                                                                             \
		if (a_ == NULL || strcmp(a_, e_) != 0) {                                                   \
			g_tests_failed++;                                                                      \
			printf("  FAIL %s:%d: %s is \"%s\", expected \"%s\"\n", __FILE__, __LINE__, #actual,   \
			       a_ ? a_ : "(null)", e_);                                                        \
		}                                                                                          \
	} while (0)

#define RUN(test)                                                                                  \
	do {                                                                                           \
		printf("%s\n", #test);                                                                     \
		test();                                                                                    \
	} while (0)

#endif
