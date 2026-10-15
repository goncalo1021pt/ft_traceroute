#include "ft_traceroute.h"

int main(int argc, char **argv) {
	int exit_code;
	t_traceroute traceroute;
	t_options options;

	initialize_program(&options, &traceroute);
	exit_code = parse_options(argc, argv, &traceroute, &options);
	if (exit_code == OPT_HELP) {
		print_help();
		exit_code = 0;
	} else if (exit_code == 0) {
		exit_code = exec_traceroute(&traceroute, &options);
	}
	free_options(&options);
	return exit_code;
}
