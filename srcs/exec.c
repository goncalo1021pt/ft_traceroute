#include "ft_traceroute.h"

void print_traceroute_header(t_traceroute *traceroute, const t_options *options) {
	char ip_str[INET_ADDRSTRLEN];

	inet_ntop(AF_INET, &traceroute->dest_addr.sin_addr, ip_str, sizeof(ip_str));

	ft_printf("ft_traceroute to %s (%s), %d hops max, %d byte packets\n", traceroute->hostname,
	          ip_str, options->max_hops, PACKET_SIZE);
}

#ifdef FT_BONUS
/* -t, -i: options that act on the sockets themselves (-s is applied when binding) */
static int apply_socket_options(const t_traceroute *traceroute, const t_options *options) {
	if (options->tos && set_socket_tos(traceroute->send_sockfd, options->tos) < 0)
		return -1;
	if (options->interface &&
	    (bind_socket_device(traceroute->send_sockfd, options->interface) < 0 ||
	     bind_socket_device(traceroute->sockfd, options->interface) < 0))
		return -1;
	return 0;
}
#endif

int init_traceroute(t_traceroute *traceroute, const t_options *options) {

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

#ifdef FT_BONUS
	if (apply_socket_options(traceroute, options) < 0 ||
	    bind_udp_socket(traceroute->send_sockfd, options->src_addr, &traceroute->src_port) < 0 ||
	    set_socket_timestamps(traceroute->sockfd) < 0) {
#else
	if (bind_udp_socket(traceroute->send_sockfd, NULL, &traceroute->src_port) < 0 ||
	    set_socket_timestamps(traceroute->sockfd) < 0) {
#endif
		close_socket(traceroute->sockfd);
		close_socket(traceroute->send_sockfd);
		return -1;
	}

	traceroute->current_ttl = options->first_ttl;
	traceroute->dest_port = options->port;

	return 0;
}

int perform_traceroute(t_traceroute *traceroute, const t_options *options) {
	print_traceroute_header(traceroute, options);
	return trace_route(traceroute, options);
}

void clear_traceroute(t_traceroute *traceroute, t_options *options) {
	(void)options;

	close_socket(traceroute->sockfd);
	close_socket(traceroute->send_sockfd);

	traceroute->sockfd = -1;
	traceroute->send_sockfd = -1;
}

int exec_traceroute(t_traceroute *traceroute, t_options *options) {
	int status;

	if (init_traceroute(traceroute, options))
		return EXIT_ERROR;
	status = perform_traceroute(traceroute, options);
	clear_traceroute(traceroute, options);
	return status < 0 ? EXIT_ERROR : 0;
}
