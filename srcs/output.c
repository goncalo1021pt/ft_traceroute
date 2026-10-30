#include "ft_traceroute.h"

void get_hostname(const struct sockaddr_in *addr, char *hostname, size_t len, char *ip_str) {
	int ret;

	ret = getnameinfo((const struct sockaddr *)addr, (socklen_t)sizeof(*addr), hostname,
	                  (socklen_t)len, NULL, 0, 0);

	if (ret != 0 || strcmp(hostname, ip_str) == 0) {
		ft_strcpy(hostname, ip_str);
	}
}

/* Prints the line of one finished hop. All the measuring is done by now, so the
 * (possibly slow) hostname lookups cannot change any RTT. */
void print_hop(int ttl, const t_probe *probes, int nprobes) {
	char last_ip[INET_ADDRSTRLEN] = {0};
	bool previous_was_star = false;

	printf("%2d", ttl);
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
			char marker[16];

			format_marker(probe->icmp_type, probe->icmp_code, marker, sizeof(marker));
			inet_ntop(AF_INET, &probe->from.sin_addr, ip_str, sizeof(ip_str));
			if (strcmp(ip_str, last_ip) != 0) {
				char hostname[NI_MAXHOST];

				get_hostname(&probe->from, hostname, sizeof(hostname), ip_str);
				printf("%s (%s)  ", hostname, ip_str);
				ft_strcpy(last_ip, ip_str);
			}
			printf("%.3f ms%s", probe->rtt, marker);
		}
		previous_was_star = is_star;
	}
	printf("\n");
	fflush(stdout);
}
