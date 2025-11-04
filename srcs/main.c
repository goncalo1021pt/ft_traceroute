#include "ft_traceroute.h"


int main(int argc, char **argv)
{
    int exit_code;
    t_traceroute traceroute;
    t_options options;

    initialize_program(&options, &traceroute);
    if (parse_options(argc, argv, &traceroute, &options) != 0) {
        print_help();
        return 1;
    }
    exit_code = exec_traceroute(&traceroute, &options);
    return exit_code;
}
