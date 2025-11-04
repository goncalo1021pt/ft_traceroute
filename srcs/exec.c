#include "ft_traceroute.h"

int init_traceroute(t_traceroute *traceroute, t_options *options) {
    (void)traceroute;
    (void)options;
    return 0;
}

int perform_traceroute(t_traceroute *traceroute, t_options *options) {
    (void)traceroute;
    (void)options;
    return 0;
}

void clear_traceroute(t_traceroute *traceroute, t_options *options) {
    (void)traceroute;
    (void)options;
}

int exec_traceroute(t_traceroute *traceroute, t_options *options) {
    (void)traceroute;
    (void)options;
    int exit_code;
    exit_code = init_traceroute(traceroute, options);
    if (!exit_code)
        return exit_code;
    exit_code = perform_traceroute(traceroute, options);
    clear_traceroute(traceroute, options);
    return exit_code;
}