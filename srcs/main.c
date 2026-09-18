#include "ft_traceroute.h"


int main(int argc, char **argv)
{
    int exit_code;
    t_traceroute traceroute;
    t_options options;

    initialize_program(&options, &traceroute);
    exit_code = parse_options(argc, argv, &traceroute, &options);
    if (exit_code == OPT_HELP) {
        print_help();
        return 0;
    }
    if (exit_code != 0) {
        return exit_code;
    }
    return exec_traceroute(&traceroute, &options);
}
