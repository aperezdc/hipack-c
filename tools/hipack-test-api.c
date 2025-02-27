/*
 * SPDX-License-Identifier: MIT
 */

#include <stdlib.h>
#include "../hipack.h"

enum test_result {
	TEST_PASS,
	TEST_FAIL,
	TEST_SKIP,
};

static inline enum test_result
check_log_failure(const char *file, unsigned line, const char *func, const char *message)
{
	const char *basename = strrchr(file, '/');
	if (!basename) basename = file;
	fprintf(stderr, "  %s (%s:%u): %s\n", func, file, line, message);
	fflush(stderr);
	return TEST_FAIL;
}

#define TEST(name) static enum test_result test_ ## name(void)
#define check(expr) do { \
		if (!(expr)) \
			return check_log_failure(__FILE__, __LINE__, __func__, "check failed: " #expr); \
	} while (false)

#define cleanup(kind) __attribute__((cleanup(cleanup_ ## kind)))

static inline void
cleanup_value(hipack_value_t *valp)
{
	if (valp)
		hipack_value_free(valp);
}

TEST(value_equal)
{
	hipack_value_t a cleanup(value) = hipack_integer(42);
	hipack_value_t b cleanup(value) = hipack_integer(32);
	hipack_value_t c cleanup(value) = hipack_string(hipack_string_new_from_string("Hi there!"));
	hipack_value_t d cleanup(value) = hipack_float(3.14);
	hipack_value_t e cleanup(value) = hipack_dict(hipack_dict_new());
	hipack_value_t f cleanup(value) = hipack_list(hipack_list_new(0));
	hipack_value_t g cleanup(value) = hipack_list(hipack_list_new(0));

	check(hipack_value_equal(&a, &a));
	check(hipack_value_equal(&b, &b));
	check(hipack_value_equal(&c, &c));
	check(hipack_value_equal(&d, &d));
	check(hipack_value_equal(&e, &e));
	check(hipack_value_equal(&f, &f));
	check(hipack_value_equal(&f, &g));

	check(!hipack_value_equal(&a, &b));
	check(!hipack_value_equal(&a, &c));
	check(!hipack_value_equal(&a, &d));
	check(!hipack_value_equal(&a, &e));

	return TEST_PASS;
}

TEST(list_equal)
{
	hipack_list_t *l0 = hipack_list_new(0);
	hipack_list_t *l1 = hipack_list_new(1);
	hipack_list_t *l3 = hipack_list_new(3);
	hipack_list_t *lI = hipack_list_new(1);

	hipack_value_t v0 cleanup(value) = hipack_list(l0);
	hipack_value_t v1 cleanup(value) = hipack_list(l1);
	hipack_value_t v3 cleanup(value) = hipack_list(l3);
	hipack_value_t vI cleanup(value) = hipack_list(lI);

	*HIPACK_LIST_AT(l1, 0) = hipack_integer(22);
	*HIPACK_LIST_AT(lI, 0) = hipack_integer(22);

	*HIPACK_LIST_AT(l3, 0) = hipack_bool(true);
	*HIPACK_LIST_AT(l3, 2) = hipack_bool(true);
	*HIPACK_LIST_AT(l3, 2) = hipack_bool(false);

	check(hipack_list_equal(l0, l0));
	check(hipack_list_equal(l3, l3));
	check(hipack_list_equal(l1, l1));
	check(hipack_list_equal(l1, lI));
	check(!hipack_list_equal(l0, l1));
	check(!hipack_list_equal(l0, l3));
	check(!hipack_list_equal(l0, lI));

	check(hipack_value_equal(&v0, &v0));
	check(!hipack_value_equal(&v0, &v1));

	return TEST_PASS;
}

#undef TEST

static size_t stat_skipped = 0;
static size_t stat_passed = 0;
static size_t stat_failed = 0;

static void
run_test(const char *name, enum test_result (*run)(void))
{
	fprintf(stderr, "%s...\n", name);
	fflush(stderr);

	const char *status;
	switch ((*run)()) {
		case TEST_PASS:
			stat_passed++;
			status = "ok";
			break;
		case TEST_FAIL:
			stat_failed++;
			status = "FAILED";
			break;
		case TEST_SKIP:
			stat_skipped++;
			status = "skipped";
			break;
	}
	
	fprintf(stderr, "%s... %s\n", name, status);
	fflush(stderr);
}

int
main(int argc, char *argv[])
{
	static const struct {
		const char *name;
		enum test_result (*run)(void);
	} tests[] = {
#define TEST(name) { #name, test_ ## name }
		TEST(value_equal),
		TEST(list_equal),
#undef TEST
	};

	if (argc > 1) {
		for (size_t j = 1; j < argc; j++) {
			for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
				if (!strcmp(argv[j], tests[i].name)) {
					run_test(tests[i].name, tests[i].run);
					goto next;
				}
			}
			fprintf(stderr, "%s: unknown test '%s'\n", argv[0], argv[j]);
next:
			(void)0;
		}
	} else {
		for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
			run_test(tests[i].name, tests[i].run);
	}

	const size_t stat_run = stat_passed + stat_failed;
	const size_t stat_total = stat_passed + stat_skipped + stat_failed;

	printf("Passed %zu/%zu tests (%.2f%%) - %zu ok, %zu skipped, %zu failed.\n",
			stat_run, stat_total, 100.0 * (double) stat_run / stat_total,
			stat_passed, stat_skipped, stat_failed);

	return stat_failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
