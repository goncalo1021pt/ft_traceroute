#include "ft_traceroute.h"


int main(int argc, char **argv)
{
    int exit_code;
    t_traceroute traceroute;
    t_options options;

    initialize_program(&options, &traceroute);
    exit_code = parse_options(argc, argv, &traceroute, &options);
    if (exit_code != 0) {
        print_help();
        return 1;
    }
    return exec_traceroute(&traceroute, &options);
}
