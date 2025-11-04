#include "ft_traceroute.h"

void initialize_program(t_options *options, t_traceroute *traceroute) {
    ft_bzero(options, sizeof(t_options));
	ft_bzero(traceroute, sizeof(t_traceroute));
}

void print_options(t_options *options) {
    (void)options;
}

void print_help() {
}

int parse_options(int argc, char **argv, t_traceroute *traceroute, t_options *options) {
	int opt;
	
	if (argc < 2) {
		fprintf(stderr, "ft_traceroute: usage error: Destination address required\n");
		return 1; 
	}
	while ((opt = getopt(argc, argv, "")) != -1) {
		switch (opt) {
			case 'c':
				break;
			case '?':
				fprintf(stderr, "\n");
				options->help = true;
				return 2;
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
