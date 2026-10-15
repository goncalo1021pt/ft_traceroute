#include "ft_traceroute.h"
#include "test.h"

int g_tests_run;
int g_tests_failed;

/* parse_options() relies on getopt_long, whose state is global: reset it every call. */
static int parse(t_traceroute *tr, t_options *opts, int argc, char **argv) {
	initialize_program(opts, tr);
	optind = 0;
	return parse_options(argc, argv, tr, opts);
}

static void test_defaults(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 2, argv), 0);
	CHECK_STR(tr.ip_address, "127.0.0.1");
	CHECK_INT(opts.max_hops, 30);
	CHECK_INT(opts.first_ttl, 1);
	CHECK_INT(opts.nqueries, 3);
	CHECK_INT(opts.waittime, 5);
	CHECK_INT(opts.port, 33434);
	CHECK_INT(opts.help, 0);
	CHECK(opts.interface == NULL);
	CHECK(opts.src_addr == NULL);
}

static void test_no_arguments(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", NULL};

	CHECK_INT(parse(&tr, &opts, 1, argv), 1);
}

static void test_missing_destination(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-q", "2", NULL};

	CHECK_INT(parse(&tr, &opts, 3, argv), 1);
}

static void test_help(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "--help", NULL};

	CHECK_INT(parse(&tr, &opts, 2, argv), OPT_HELP);
	CHECK_INT(opts.help, 1);
}

static void test_unknown_option(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-Z", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 3, argv), 1);
}

static void test_numeric_options(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-m", "12", "-p", "4000", "-q", "5", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 8, argv), 0);
	CHECK_INT(opts.max_hops, 12);
	CHECK_INT(opts.port, 4000);
	CHECK_INT(opts.nqueries, 5);
	CHECK_STR(tr.ip_address, "127.0.0.1");
}

static void test_long_options(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "--max-hops=7", "--queries", "4",
	                "--port=5000",   "10.0.0.1",     NULL};

	CHECK_INT(parse(&tr, &opts, 6, argv), 0);
	CHECK_INT(opts.max_hops, 7);
	CHECK_INT(opts.nqueries, 4);
	CHECK_INT(opts.port, 5000);
	CHECK_STR(tr.ip_address, "10.0.0.1");
}

static void test_string_options(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-i", "eth0", "-s", "10.0.0.2", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 6, argv), 0);
	CHECK_STR(opts.interface, "eth0");
	CHECK_STR(opts.src_addr, "10.0.0.2");
	free_options(&opts);
}

static void test_repeated_string_option(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-i", "eth0", "-i", "eth1", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 6, argv), 0);
	CHECK_STR(opts.interface, "eth1");
	free_options(&opts);
}

static void test_options_after_destination(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "127.0.0.1", "-q", "2", NULL};

	CHECK_INT(parse(&tr, &opts, 4, argv), 0);
	CHECK_INT(opts.nqueries, 2);
	CHECK_STR(tr.ip_address, "127.0.0.1");
}

static void test_queries_bounds(void) {
	t_traceroute tr;
	t_options opts;
	char *ok_min[] = {"ft_traceroute", "-q", "1", "127.0.0.1", NULL};
	char *ok_max[] = {"ft_traceroute", "-q", "10", "127.0.0.1", NULL};
	char *zero[] = {"ft_traceroute", "-q", "0", "127.0.0.1", NULL};
	char *above[] = {"ft_traceroute", "-q", "11", "127.0.0.1", NULL};
	char *negative[] = {"ft_traceroute", "-q", "-3", "127.0.0.1", NULL};
	char *text[] = {"ft_traceroute", "-q", "abc", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 4, ok_min), 0);
	CHECK_INT(opts.nqueries, 1);
	CHECK_INT(parse(&tr, &opts, 4, ok_max), 0);
	CHECK_INT(opts.nqueries, MAX_QUERIES);

	CHECK_INT(parse(&tr, &opts, 4, zero), 1);
	CHECK_INT(parse(&tr, &opts, 4, above), 1);
	CHECK_INT(parse(&tr, &opts, 4, negative), 1);
	CHECK_INT(parse(&tr, &opts, 4, text), 1);
}

static void test_rejected_queries_keep_default(void) {
	t_traceroute tr;
	t_options opts;
	char *argv[] = {"ft_traceroute", "-q", "99", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 4, argv), 1);
	CHECK_INT(opts.nqueries, 3);
}

static void test_max_hops_validation(void) {
	t_traceroute tr;
	t_options opts;
	char *ok_min[] = {"ft_traceroute", "-m", "1", "127.0.0.1", NULL};
	char *ok_max[] = {"ft_traceroute", "-m", "255", "127.0.0.1", NULL};
	char *zero[] = {"ft_traceroute", "-m", "0", "127.0.0.1", NULL};
	char *negative[] = {"ft_traceroute", "-m", "-1", "127.0.0.1", NULL};
	char *above[] = {"ft_traceroute", "-m", "256", "127.0.0.1", NULL};
	char *text[] = {"ft_traceroute", "-m", "abc", "127.0.0.1", NULL};
	char *trailing[] = {"ft_traceroute", "-m", "5abc", "127.0.0.1", NULL};
	char *empty[] = {"ft_traceroute", "-m", "", "127.0.0.1", NULL};
	char *space[] = {"ft_traceroute", "-m", " 5", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 4, ok_min), 0);
	CHECK_INT(opts.max_hops, 1);
	CHECK_INT(parse(&tr, &opts, 4, ok_max), 0);
	CHECK_INT(opts.max_hops, 255);
	CHECK_INT(parse(&tr, &opts, 4, zero), 1);
	CHECK_INT(parse(&tr, &opts, 4, negative), 1);
	CHECK_INT(parse(&tr, &opts, 4, above), 1);
	CHECK_INT(parse(&tr, &opts, 4, text), 1);
	CHECK_INT(parse(&tr, &opts, 4, trailing), 1);
	CHECK_INT(parse(&tr, &opts, 4, empty), 1);
	CHECK_INT(parse(&tr, &opts, 4, space), 1);
}

static void test_port_validation(void) {
	t_traceroute tr;
	t_options opts;
	char *ok[] = {"ft_traceroute", "-p", "60000", "127.0.0.1", NULL};
	char *last[] = {"ft_traceroute", "-p", "65535", "-m", "1", "-q", "1", "127.0.0.1", NULL};
	char *zero[] = {"ft_traceroute", "-p", "0", "127.0.0.1", NULL};
	char *above[] = {"ft_traceroute", "-p", "99999", "127.0.0.1", NULL};
	char *negative[] = {"ft_traceroute", "-p", "-5", "127.0.0.1", NULL};
	char *text[] = {"ft_traceroute", "-p", "http", "127.0.0.1", NULL};
	/* valid on its own, but 30 hops x 3 probes would run past port 65535 */
	char *overflow[] = {"ft_traceroute", "-p", "65500", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 4, ok), 0);
	CHECK_INT(opts.port, 60000);
	CHECK_INT(parse(&tr, &opts, 8, last), 0);
	CHECK_INT(opts.port, 65535);
	CHECK_INT(parse(&tr, &opts, 4, zero), 1);
	CHECK_INT(parse(&tr, &opts, 4, above), 1);
	CHECK_INT(parse(&tr, &opts, 4, negative), 1);
	CHECK_INT(parse(&tr, &opts, 4, text), 1);
	CHECK_INT(parse(&tr, &opts, 4, overflow), 1);
}

static void test_numbers_that_wrap(void) {
	t_traceroute tr;
	t_options opts;
	char *wraps_q[] = {"ft_traceroute", "-q", "4294967299", "127.0.0.1", NULL};
	char *wraps_m[] = {"ft_traceroute", "-m", "4294967297", "127.0.0.1", NULL};
	char *huge[] = {"ft_traceroute", "-q", "99999999999999999999", "127.0.0.1", NULL};
	char *trailing_q[] = {"ft_traceroute", "-q", "5abc", "127.0.0.1", NULL};

	CHECK_INT(parse(&tr, &opts, 4, wraps_q), 1);
	CHECK_INT(parse(&tr, &opts, 4, wraps_m), 1);
	CHECK_INT(parse(&tr, &opts, 4, huge), 1);
	CHECK_INT(parse(&tr, &opts, 4, trailing_q), 1);
}

int main(void) {
	RUN(test_defaults);
	RUN(test_no_arguments);
	RUN(test_missing_destination);
	RUN(test_help);
	RUN(test_unknown_option);
	RUN(test_numeric_options);
	RUN(test_long_options);
	RUN(test_string_options);
	RUN(test_repeated_string_option);
	RUN(test_options_after_destination);
	RUN(test_queries_bounds);
	RUN(test_rejected_queries_keep_default);
	RUN(test_max_hops_validation);
	RUN(test_port_validation);
	RUN(test_numbers_that_wrap);

	printf("\n%d checks, %d failed\n", g_tests_run, g_tests_failed);
	return g_tests_failed != 0;
}
