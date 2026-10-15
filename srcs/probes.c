#include "ft_traceroute.h"

void get_hostname(const struct sockaddr_in *addr, char *hostname, size_t len, char *ip_str) {
	int ret;

	ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)sizeof(*addr), hostname,
	                  (socklen_t)len, NULL, 0, 0);

	if (ret != 0 || strcmp(hostname, ip_str) == 0) {
		ft_strcpy(hostname, ip_str);
	}
}

int send_single_probe(t_traceroute *traceroute, t_probe *probe) {
	char packet[PAYLOAD_SIZE];
	struct sockaddr_in dest;
	ssize_t sent;

	if (set_socket_ttl(traceroute->send_sockfd, traceroute->current_ttl) < 0) {
		ft_fprintf(2, "Failed to set TTL\n");
		return -1;
	}

	ft_memcpy(&dest, &traceroute->dest_addr, sizeof(dest));
	dest.sin_port = htons((uint16_t)(traceroute->dest_port + traceroute->sequence));

	ft_bzero(packet, sizeof(packet));
	*(uint16_t *)packet = htons(traceroute->sequence);

	gettimeofday(&probe->sent, NULL);

	sent = sendto(traceroute->send_sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&dest,
	              sizeof(dest));
	if (sent < 0) {
		perror("sendto");
		return -1;
	}
	traceroute->sequence++;
	traceroute->stats.packets_sent++;

	return 0;
}

/* Waits for the reply to `probe` and stores who answered and the RTT in it.
 * Returns the ICMP type, or -1 on timeout. The RTT is taken the moment the packet
 * arrives, before anything slow (like a reverse DNS lookup) can happen. */
int receive_single_reply(t_traceroute *traceroute, const t_options *options, t_probe *probe) {
	struct sockaddr_in *from = &probe->from;
	unsigned char buffer[1024];
	socklen_t from_len = sizeof(*from);
	struct timeval recv_time, timeout, start_time;
	fd_set readfds;
	struct icmphdr *icmp;

	gettimeofday(&start_time, NULL);

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(traceroute->sockfd, &readfds);

		struct timeval now, elapsed;
		gettimeofday(&now, NULL);
		timersub(&now, &start_time, &elapsed);

		timeout.tv_sec = options->waittime - elapsed.tv_sec;
		timeout.tv_usec = -elapsed.tv_usec;

		if (timeout.tv_usec < 0) {
			timeout.tv_sec--;
			timeout.tv_usec += 1000000;
		}

		if (timeout.tv_sec < 0)
			return -1;

		int ret = select(traceroute->sockfd + 1, &readfds, NULL, NULL, &timeout);
		if (ret <= 0) {
			if (ret < 0)
				perror("select");
			return -1;
		}

		gettimeofday(&recv_time, NULL);
		ssize_t bytes_received = recvfrom(traceroute->sockfd, buffer, sizeof(buffer), 0,
		                                  (struct sockaddr *)from, &from_len);
		if (bytes_received < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			perror("recvfrom");
			return -1;
		}

		const struct ip *ip_hdr = (struct ip *)buffer;
		int ip_hdr_len = ip_hdr->ip_hl << 2;

		icmp = (struct icmphdr *)(buffer + ip_hdr_len);

		if (icmp->type == ICMP_TIME_EXCEEDED || icmp->type == ICMP_DEST_UNREACH) {
			struct ip *orig_ip = (struct ip *)((char *)icmp + sizeof(struct icmphdr));

			if (orig_ip->ip_p == IPPROTO_UDP) {
				struct udphdr *orig_udp =
				    (struct udphdr *)((char *)orig_ip + (orig_ip->ip_hl << 2));
				uint16_t orig_port = ntohs(orig_udp->uh_dport);

				if (orig_port >= traceroute->dest_port && orig_port < traceroute->dest_port + 100) {
					probe->rtt = (double)(recv_time.tv_sec - probe->sent.tv_sec) * 1000.0 +
					             (double)(recv_time.tv_usec - probe->sent.tv_usec) / 1000.0;

					traceroute->stats.packets_received++;
					return icmp->type;
				}
			}
		}
	}
}

/* Prints the results of one hop. All the measuring is already done here, so the
 * (possibly slow) hostname lookups cannot change any RTT. */
static void print_hop_results(const t_probe *probes, int nprobes) {
	char last_ip[INET_ADDRSTRLEN] = {0};
	bool previous_was_star = false;

	for (int i = 0; i < nprobes; i++) {
		const t_probe *probe = &probes[i];
		bool is_star = !probe->replied;

		if (i == 0)
			printf("  ");
		else
			printf(is_star || previous_was_star ? " " : "  ");
		if (is_star) {
			printf("*");
		} else {
			char ip_str[INET_ADDRSTRLEN];

			inet_ntop(AF_INET, &probe->from.sin_addr, ip_str, sizeof(ip_str));
			if (strcmp(ip_str, last_ip) != 0) {
				char hostname[NI_MAXHOST];

				get_hostname(&probe->from, hostname, sizeof(hostname), ip_str);
				printf("%s (%s)  ", hostname, ip_str);
				ft_strcpy(last_ip, ip_str);
			}
			printf("%.3f ms", probe->rtt);
		}
		previous_was_star = is_star;
	}
	printf("\n");
	fflush(stdout);
}

int send_probes(t_traceroute *traceroute, const t_options *options) {
	t_probe probes[MAX_QUERIES];
	int reached_dest = 0;

	ft_bzero(probes, sizeof(probes));
	printf("%2d", traceroute->current_ttl);
	fflush(stdout);

	for (int i = 0; i < options->nqueries; i++)
		probes[i].sent_ok = send_single_probe(traceroute, &probes[i]) == 0;

	for (int i = 0; i < options->nqueries; i++) {
		if (!probes[i].sent_ok)
			continue;
		probes[i].icmp_type = receive_single_reply(traceroute, options, &probes[i]);
		probes[i].replied = probes[i].icmp_type >= 0;
		if (probes[i].icmp_type == ICMP_DEST_UNREACH)
			reached_dest = 1;
	}

	print_hop_results(probes, options->nqueries);
	return reached_dest;
}
