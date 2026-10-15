#include "ft_traceroute.h"

void initialize_program(t_options *options, t_traceroute *traceroute) {
	ft_bzero(options, sizeof(t_options));
	ft_bzero(traceroute, sizeof(t_traceroute));

	options->max_hops = 30;
	options->first_ttl = 1;
	options->nqueries = 3;
	options->waittime = 5;
	options->port = 33434;

	traceroute->sockfd = -1;
	traceroute->send_sockfd = -1;
	traceroute->identifier = (uint16_t)(getpid() & 0xFFFF);
	traceroute->sequence = 0;
	traceroute->current_ttl = 1;
}

void free_options(t_options *options) {
	free(options->interface);
	free(options->src_addr);
	free(options->tos);
	free(options->flowlabel);
}

void print_options(t_options *options) {
	(void)options;
}

void print_help(void) {
	printf("Usage: ft_traceroute [options] <destination>\n");
	printf("Options:\n");
	printf("  -i, --interface <interface>   Interface to use\n");
	printf("  -m, --max-hops <hops>         Maximum number of hops\n");
	printf("  -p, --port <port>             Port to use\n");
	printf("  -s, --source <address>        Source address\n");
	printf("  -q, --queries <queries>       Number of queries per hop\n");
	printf("  -N, --sim-queries <queries>   Number of simultaneous queries\n");
	printf("  -t, --tos <tos>               Type of service\n");
	printf("  -l, --flowlabel <label>       Flow label\n");
	printf("  --help                        Show this help message\n");
}

/* Strict decimal parser: the whole string must be a number inside [min, max]. */
static bool parse_number(const char *str, long min, long max, long *out) {
	char *end;
	long value;

	if (str[0] != '-' && str[0] != '+' && !(str[0] >= '0' && str[0] <= '9'))
		return false;
	errno = 0;
	value = strtol(str, &end, 10);
	if (errno == ERANGE || end == str || *end != '\0' || value < min || value > max)
		return false;
	*out = value;
	return true;
}

/* Parses an option argument into *out, printing "<message> `<arg>'" on failure. */
static int parse_int_option(const char *arg, long min, long max, int *out, const char *message) {
	long value;

	if (!parse_number(arg, min, max, &value)) {
		fprintf(stderr, "ft_traceroute: %s `%s'\n", message, arg);
		return 1;
	}
	*out = (int)value;
	return 0;
}

int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options) {
	int opt;
	struct option long_options[] = {
	    {"interface", required_argument, NULL, 'i'}, {"max-hops", required_argument, NULL, 'm'},
	    {"port", required_argument, NULL, 'p'},      {"source", required_argument, NULL, 's'},
	    {"queries", required_argument, NULL, 'q'},   {"sim-queries", required_argument, NULL, 'N'},
	    {"tos", required_argument, NULL, 't'},       {"flowlabel", required_argument, NULL, 'l'},
	    {"help", no_argument, NULL, OPT_HELP},       {NULL, 0, NULL, 0}};

	if (argc < 2) {
		fprintf(stderr, "ft_traceroute: usage error: Destination address required\n");
		return 1;
	}
	while ((opt = getopt_long(argc, argv, "i:m:p:s:q:N:t:l:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'i':
				free(options->interface);
				options->interface = ft_strdup(optarg);
				break;
			case 'm':
				if (parse_int_option(optarg, 1, MAX_HOPS, &options->max_hops, "invalid hops value"))
					return 1;
				break;
			case 'p':
				if (parse_int_option(optarg, 1, MAX_PORT, &options->port, "invalid port value"))
					return 1;
				break;
			case 's':
				free(options->src_addr);
				options->src_addr = ft_strdup(optarg);
				break;
			case 'q': {
				long nqueries;

				if (!parse_number(optarg, 1, MAX_QUERIES, &nqueries)) {
					fprintf(stderr, "ft_traceroute: no more than %d probes per hop\n", MAX_QUERIES);
					return 1;
				}
				options->nqueries = (int)nqueries;
				break;
			}
			case 'N':
				if (parse_int_option(optarg, 1, MAX_HOPS, &options->squeries,
				                     "invalid simultaneous probes value"))
					return 1;
				break;
			case 't':
				free(options->tos);
				options->tos = ft_strdup(optarg);
				break;
			case 'l':
				free(options->flowlabel);
				options->flowlabel = ft_strdup(optarg);
				break;
			case OPT_HELP:
				options->help = true;
				return OPT_HELP;
			default:
				fprintf(stderr, "ft_traceroute: usage error: Destination address required\n");
				return 1;
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "ft_traceroute: usage error: Destination address required\n");
		return 1;
	}
	/* every probe uses its own destination port, so they must all fit below 65535 */
	if (options->port + options->max_hops * options->nqueries - 1 > MAX_PORT) {
		fprintf(stderr, "ft_traceroute: port %d is too high for %d hops of %d probes\n",
		        options->port, options->max_hops, options->nqueries);
		return 1;
	}
	traceroute->ip_address = argv[optind];
	return 0;
}
