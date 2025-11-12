#include "ft_traceroute.h"

void get_hostname(struct sockaddr_in *addr, char *hostname, size_t len, char *ip_str) {
	int ret;
	
	ret = getnameinfo((struct sockaddr *)addr, sizeof(*addr), hostname, len, NULL, 0, 0);
	
	if (ret != 0 || strcmp(hostname, ip_str) == 0) {
		ft_strcpy(hostname, ip_str);
	}
}

int send_single_probe(t_traceroute *traceroute) {
	char packet[PACKET_SIZE];
	struct sockaddr_in dest;
	ssize_t sent;
	
	if (set_socket_ttl(traceroute->send_sockfd, traceroute->current_ttl) < 0) {
		ft_fprintf(2, "Failed to set TTL\n");
		return -1;
	}
	
	ft_memcpy(&dest, &traceroute->dest_addr, sizeof(dest));
	dest.sin_port = htons(traceroute->dest_port + traceroute->sequence);
	
	ft_bzero(packet, sizeof(packet));
	*(uint16_t *)packet = htons(traceroute->sequence);
	
	gettimeofday(&traceroute->stats.last_send_time, NULL);
	
	sent = sendto(traceroute->send_sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(dest));
	if (sent < 0) {
		perror("sendto");
		return -1;
	}
	traceroute->sequence++;
	traceroute->stats.packets_sent++;
	
	return 0;
}

int receive_single_reply(t_traceroute *traceroute, t_options *options, struct sockaddr_in *from, double *rtt) {
	unsigned char buffer[1024];
	ssize_t bytes_received;
	socklen_t from_len = sizeof(*from);
	struct timeval recv_time, timeout, start_time;
	fd_set readfds;
	int ret;
	struct icmphdr *icmp;
	
	gettimeofday(&start_time, NULL);

	while (1)
	{
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

		ret = select(traceroute->sockfd + 1, &readfds, NULL, NULL, &timeout);
		if (ret <= 0) {
			if (ret < 0)
				perror("select");
			return -1;
		}
		
		gettimeofday(&recv_time, NULL);
		bytes_received = recvfrom(traceroute->sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)from, &from_len);
		if (bytes_received < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			perror("recvfrom");
			return -1;
		}
		
		struct ip *ip_hdr = (struct ip *)buffer;
		int ip_hdr_len = ip_hdr->ip_hl << 2;
		
		icmp = (struct icmphdr *)(buffer + ip_hdr_len);
		
		if (icmp->type == ICMP_TIME_EXCEEDED || icmp->type == ICMP_DEST_UNREACH) {
			struct ip *orig_ip = (struct ip *)((char *)icmp + sizeof(struct icmphdr));

			if (orig_ip->ip_p == IPPROTO_UDP) {
				struct udphdr *orig_udp = (struct udphdr *)((char *)orig_ip + (orig_ip->ip_hl << 2));
				uint16_t orig_port = ntohs(orig_udp->uh_dport);

				if (orig_port >= traceroute->dest_port && 
					orig_port < traceroute->dest_port + 100) {
					*rtt = (recv_time.tv_sec - traceroute->stats.last_send_time.tv_sec) * 1000.0 +
						   (recv_time.tv_usec - traceroute->stats.last_send_time.tv_usec) / 1000.0;
					
					traceroute->stats.packets_received++;
					return icmp->type;
				}
			}
		}
	}
}

int send_probes(t_traceroute *traceroute, t_options *options) {
	int i;
	int reached_dest = 0;
	struct timeval send_times[options->nqueries];
	
	if (traceroute->current_ttl < 10)
		printf(" %d  ", traceroute->current_ttl);
	else
		printf("%d  ", traceroute->current_ttl);
	
	for (i = 0; i < options->nqueries; i++) {
		if (send_single_probe(traceroute) < 0) {
			send_times[i].tv_sec = 0;
			send_times[i].tv_usec = 0;
		} else {
			send_times[i] = traceroute->stats.last_send_time;
		}
	}
	
	for (i = 0; i < options->nqueries; i++) {
		struct sockaddr_in from;
		double rtt;
		int icmp_type;
		char ip_str[INET_ADDRSTRLEN];
		char hostname[NI_MAXHOST];
		static char last_ip[INET_ADDRSTRLEN] = {0};
		static int first_response = 1;
		
		if (send_times[i].tv_sec == 0) {
			printf("* ");
			continue;
		}
		
		traceroute->stats.last_send_time = send_times[i];
		
		icmp_type = receive_single_reply(traceroute, options, &from, &rtt);
		
		if (icmp_type < 0) {
			printf("* ");
			first_response = 1;
		} else {
			inet_ntop(AF_INET, &from.sin_addr, ip_str, sizeof(ip_str));
			
			if (first_response || strcmp(ip_str, last_ip) != 0) {
				get_hostname(&from, hostname, sizeof(hostname), ip_str);
				printf("%s (%s)  %.3f ms  ", hostname, ip_str, rtt);
				ft_strcpy(last_ip, ip_str);
				first_response = 0;
			} else 
				printf("%.3f ms  ", rtt);
			
			if (icmp_type == ICMP_DEST_UNREACH) {
				reached_dest = 1;
			}
		}
	}
	printf("\n");
	fflush(stdout);
	return reached_dest;
}

