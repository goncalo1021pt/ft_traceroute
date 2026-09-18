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
	traceroute->identifier = getpid() & 0xFFFF;
	traceroute->sequence = 0;
	traceroute->current_ttl = 1;
}

void print_options(t_options *options) {
    (void)options;
}

void print_help() {
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

int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options) {
	int opt;
	struct option long_options[] = {
		{"interface", required_argument, NULL, 'i'},
		{"max-hops", required_argument, NULL, 'm'},
		{"port", required_argument, NULL, 'p'},
		{"source", required_argument, NULL, 's'},
		{"queries", required_argument, NULL, 'q'},
		{"sim-queries", required_argument, NULL, 'N'},
		{"tos", required_argument, NULL, 't'},
		{"flowlabel", required_argument, NULL, 'l'},
		{"help", no_argument, NULL, OPT_HELP},
		{NULL, 0, NULL, 0}
	};
	
	if (argc < 2) {
		fprintf(stderr, "ft_traceroute: usage error: Destination address required\n");
		return 1; 
	}
	while ((opt = getopt_long(argc, argv, "i:m:p:s:q:N:t:l:", long_options, NULL)) != -1) {
		switch (opt) {
			case 'i':
				options->interface = ft_strdup(optarg);
				break;
			case 'm':
				options->max_hops = atoi(optarg);
				break;
			case 'p':
				options->port = atoi(optarg);
				break;
			case 's':
				options->src_addr = ft_strdup(optarg);
				break;
			case 'q':
				options->nqueries = atoi(optarg);
				break;
			case 'N':
				options->squeries = atoi(optarg);
				break;
			case 't':
				options->tos = ft_strdup(optarg);
				break;
			case 'l':
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
	traceroute->ip_address = argv[optind];
	return 0;
}
