#include "ft_traceroute.h"

int create_icmp_socket(void) {
	int sockfd;

	sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0) {
		if (errno == EPERM || errno == EACCES) {
			fprintf(stderr, "ft_traceroute: Operation not permitted\n");
			fprintf(stderr, "ft_traceroute: You need root privileges to create raw sockets\n");
			fprintf(stderr, "ft_traceroute: Try running with sudo\n");
		} else
			perror("ft_traceroute: socket creation failed");
		return -1;
	}
	return sockfd;
}

int create_udp_socket(void) {
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0) {
		perror("ft_traceroute: UDP socket creation failed");
		return -1;
	}
	return sockfd;
}

void close_socket(int sockfd) {
	if (sockfd >= 0) {
		close(sockfd);
	}
}

int set_socket_ttl(int sockfd, int ttl) {
	if (setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
		perror("ft_traceroute: setsockopt TTL failed");
		return -1;
	}
	return 0;
}

int set_socket_timeout(int sockfd, int seconds) {
	struct timeval timeout;
	
	timeout.tv_sec = seconds;
	timeout.tv_usec = 0;
	
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
		perror("ft_traceroute: setsockopt timeout failed");
		return -1;
	}
	return 0;
}

int resolve_address(t_traceroute *traceroute) {
	struct addrinfo hints, *result;
	int status;
	memset(&traceroute->dest_addr, 0, sizeof(traceroute->dest_addr));
	traceroute->dest_addr.sin_family = AF_INET;
	
	if (inet_aton(traceroute->ip_address, &traceroute->dest_addr.sin_addr) != 0) {
		traceroute->hostname = traceroute->ip_address;
		return 0;
	}
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	status = getaddrinfo(traceroute->ip_address, NULL, &hints, &result);
	if (status != 0) {
		ft_fprintf(2, "%s: Name or service not known\n", traceroute->ip_address);
		ft_fprintf(2, "Cannot handle \"host\" cmdline arg '%s' on position 1 (argc 1)\n", traceroute->ip_address);
		return -1;
	}
	struct sockaddr_in *addr_in = (struct sockaddr_in *)result->ai_addr;
	memcpy(&traceroute->dest_addr.sin_addr, &addr_in->sin_addr, sizeof(traceroute->dest_addr.sin_addr));
	traceroute->hostname = traceroute->ip_address;
	freeaddrinfo(result);
	return 0;
}