#include "ft_traceroute.h"

/* State of one trace. Probe number i goes to port dest_port + i and belongs to hop
 * first_ttl + i / nqueries, so a reply identifies its probe by the port it quotes. */
typedef struct s_run {
	t_traceroute *traceroute;
	const t_options *options;
	t_probe *probes;
	int nqueries;
	int next_send; /* first probe not sent yet */
	int end_probe; /* probes from here on are not needed (the destination answered before) */
	int in_flight; /* probes sent and neither answered nor timed out */
	int scan_from; /* every probe before this one is finished */
	int printed;   /* hops already printed */
} t_run;

static double elapsed_ms(const struct timeval *from, const struct timeval *to) {
	return (double)(to->tv_sec - from->tv_sec) * 1000.0 +
	       (double)(to->tv_usec - from->tv_usec) / 1000.0;
}

/* true when a is strictly later than b */
static bool is_after(const struct timeval *a, const struct timeval *b) {
	return a->tv_sec > b->tv_sec || (a->tv_sec == b->tv_sec && a->tv_usec > b->tv_usec);
}

static void finish_probe(t_run *run, t_probe *probe) {
	if (probe->state == PROBE_PENDING)
		run->in_flight--;
	probe->state = PROBE_DONE;
}

/* The destination answered at probe `index`: nothing after its hop is needed. */
static void set_end(t_run *run, int index) {
	int end = (index / run->nqueries + 1) * run->nqueries;

	if (end >= run->end_probe)
		return;
	run->end_probe = end;
	for (int i = end; i < run->next_send; i++)
		finish_probe(run, &run->probes[i]);
}

static int send_probe(t_run *run, int index) {
	t_traceroute *traceroute = run->traceroute;
	t_probe *probe = &run->probes[index];
	char payload[PAYLOAD_SIZE];
	struct sockaddr_in dest;

	probe->ttl = run->options->first_ttl + index / run->nqueries;
	probe->port = (uint16_t)(traceroute->dest_port + index);
	if (set_socket_ttl(traceroute->send_sockfd, probe->ttl) < 0)
		return -1;

	ft_memcpy(&dest, &traceroute->dest_addr, sizeof(dest));
	dest.sin_port = htons(probe->port);
	ft_bzero(payload, sizeof(payload));
	*(uint16_t *)payload = htons((uint16_t)index);

	gettimeofday(&probe->sent, NULL);
	if (sendto(traceroute->send_sockfd, payload, sizeof(payload), 0, (struct sockaddr *)&dest,
	           sizeof(dest)) < 0) {
		perror("ft_traceroute: sendto");
		return -1;
	}
	probe->state = PROBE_PENDING;
	run->in_flight++;
	traceroute->stats.packets_sent++;
	return 0;
}

/* Keeps up to -N probes in flight, hop after hop. */
static int send_ready(t_run *run) {
	while (run->next_send < run->end_probe && run->in_flight < run->options->squeries) {
		if (send_probe(run, run->next_send) < 0)
			return -1;
		run->next_send++;
	}
	return 0;
}

/* Prints, in order, every hop whose probes are all finished. */
static void print_ready(t_run *run) {
	while (run->printed * run->nqueries < run->end_probe) {
		int first = run->printed * run->nqueries;

		for (int i = first; i < first + run->nqueries; i++)
			if (run->probes[i].state != PROBE_DONE)
				return;
		print_hop(run->probes[first].ttl, &run->probes[first], run->nqueries);
		run->printed++;
	}
}

/* Reads one packet from the raw socket. Returns 1 when it is a reply to one of our probes
 * (with the sender, the time the kernel received it and what it says), 0 when it is not,
 * -1 on error. The time comes from the kernel, not from when we got around to reading, so
 * a slow reverse DNS lookup can never inflate an RTT. */
static int receive_one(t_traceroute *traceroute, struct sockaddr_in *from, struct timeval *when,
                       t_reply *reply) {
	unsigned char buffer[1024];
	char control[CMSG_SPACE(sizeof(struct timeval))];
	struct iovec iov = {buffer, sizeof(buffer)};
	struct msghdr msg;
	struct cmsghdr *cmsg;
	ssize_t bytes;

	ft_bzero(&msg, sizeof(msg));
	msg.msg_name = from;
	msg.msg_namelen = sizeof(*from);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control;
	msg.msg_controllen = sizeof(control);
	bytes = recvmsg(traceroute->sockfd, &msg, 0);
	if (bytes < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
			return 0;
		perror("ft_traceroute: recvmsg");
		return -1;
	}
	gettimeofday(when, NULL);
	for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg))
		if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP)
			ft_memcpy(when, CMSG_DATA(cmsg), sizeof(*when));
	return parse_icmp_reply(buffer, (size_t)bytes, traceroute, reply);
}

static void apply_reply(t_run *run, const struct sockaddr_in *from, const struct timeval *when,
                        const t_reply *reply) {
	int index = reply->dport - run->traceroute->dest_port;
	t_probe *probe;

	if (index < 0 || index >= run->next_send)
		return;
	probe = &run->probes[index];
	if (probe->state != PROBE_PENDING)
		return;
	probe->replied = true;
	probe->from = *from;
	probe->icmp_type = reply->type;
	probe->icmp_code = reply->code;
	probe->rtt = elapsed_ms(&probe->sent, when);
	run->traceroute->stats.packets_received++;
	finish_probe(run, probe);
	if (reply->type == ICMP_DEST_UNREACH)
		set_end(run, index);
}

/* Time left before the oldest pending probe times out (probes are sent in order, so the
 * oldest pending one is also the first to expire). */
static struct timeval time_to_next_timeout(t_run *run, const struct timeval *now) {
	struct timeval wait = {0, 0};
	struct timeval deadline;

	while (run->scan_from < run->next_send && run->probes[run->scan_from].state == PROBE_DONE)
		run->scan_from++;
	if (run->scan_from >= run->next_send)
		return wait;
	deadline = run->probes[run->scan_from].sent;
	deadline.tv_sec += run->options->waittime;
	if (is_after(&deadline, now))
		timersub(&deadline, now, &wait);
	return wait;
}

static void expire_probes(t_run *run, const struct timeval *now) {
	for (int i = run->scan_from; i < run->next_send; i++) {
		t_probe *probe = &run->probes[i];
		struct timeval deadline = probe->sent;

		deadline.tv_sec += run->options->waittime;
		if (probe->state == PROBE_PENDING && !is_after(&deadline, now))
			finish_probe(run, probe);
	}
}

static bool socket_has_data(int sockfd) {
	struct timeval zero = {0, 0};
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(sockfd, &readfds);
	return select(sockfd + 1, &readfds, NULL, NULL, &zero) > 0;
}

/* Sleeps until a reply arrives or the oldest probe times out, then handles every packet
 * already waiting before looking at any deadline: a reply that sat in the queue while we
 * were busy (printing, resolving names) must win over its own timeout. */
static int wait_for_replies(t_run *run) {
	t_traceroute *traceroute = run->traceroute;
	struct timeval now, wait;
	fd_set readfds;
	int ret;

	gettimeofday(&now, NULL);
	wait = time_to_next_timeout(run, &now);
	FD_ZERO(&readfds);
	FD_SET(traceroute->sockfd, &readfds);
	ret = select(traceroute->sockfd + 1, &readfds, NULL, NULL, &wait);
	if (ret < 0 && errno != EINTR) {
		perror("ft_traceroute: select");
		return -1;
	}
	while (ret > 0) {
		struct sockaddr_in from;
		struct timeval when;
		t_reply reply;
		int got = receive_one(traceroute, &from, &when, &reply);

		if (got < 0)
			return -1;
		if (got > 0)
			apply_reply(run, &from, &when, &reply);
		if (!socket_has_data(traceroute->sockfd))
			break;
	}
	gettimeofday(&now, NULL);
	expire_probes(run, &now);
	return 0;
}

/* Runs the whole trace: probes are sent hop after hop with at most -N in flight, every probe
 * has its own timeout, and hops are printed in order as soon as they are complete. */
int trace_route(t_traceroute *traceroute, const t_options *options) {
	t_run run;
	int nhops = options->max_hops - options->first_ttl + 1;
	int status = 0;

	ft_bzero(&run, sizeof(run));
	run.traceroute = traceroute;
	run.options = options;
	run.nqueries = options->nqueries;
	run.end_probe = nhops * options->nqueries;
	run.probes = ft_calloc((size_t)run.end_probe, sizeof(t_probe));
	if (!run.probes) {
		perror("ft_traceroute: calloc");
		return -1;
	}
	while (status == 0 && run.printed * run.nqueries < run.end_probe) {
		status = send_ready(&run);
		if (status == 0) {
			print_ready(&run);
			if (run.printed * run.nqueries < run.end_probe)
				status = wait_for_replies(&run);
		}
	}
	free(run.probes);
	return status;
}
