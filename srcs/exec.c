#include "ft_traceroute.h"

int init_traceroute(t_traceroute *traceroute, t_options *options) {

	if (resolve_address(traceroute) != 0)
		return -1;

	traceroute->sockfd = create_icmp_socket();
	if (traceroute->sockfd < 0)
		return -1;

	traceroute->send_sockfd = create_udp_socket();
	if (traceroute->send_sockfd < 0) {
		close_socket(traceroute->sockfd);
		return -1;
	}

	if (set_socket_timeout(traceroute->sockfd, options->waittime) < 0) {
		close_socket(traceroute->sockfd);
		close_socket(traceroute->send_sockfd);
		return -1;
	}

	traceroute->current_ttl = options->first_ttl;
	traceroute->dest_port = options->port;

	return 0;
}

int perform_traceroute(t_traceroute *traceroute, t_options *options) {
	(void)traceroute;
	(void)options;
	return 0;
}

void clear_traceroute(t_traceroute *traceroute, t_options *options) {
	(void)options;
	
	close_socket(traceroute->sockfd);
	close_socket(traceroute->send_sockfd);

	traceroute->sockfd = -1;
	traceroute->send_sockfd = -1;
}

int exec_traceroute(t_traceroute *traceroute, t_options *options) {
	
	if (init_traceroute(traceroute, options))
		return -1;
	perform_traceroute(traceroute, options);
	clear_traceroute(traceroute, options);
	return 0;
}