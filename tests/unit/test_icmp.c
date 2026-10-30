#include "ft_traceroute.h"
#include "test.h"

#define OUR_SPORT 40000
#define OUR_DPORT 33440

int g_tests_run;
int g_tests_failed;

/* Builds the packet the raw socket hands us: outer IP + ICMP error + the probe it quotes. */
static size_t build(unsigned char *buf, int type, int code, uint16_t sport, uint16_t dport,
                    const char *dst) {
	struct ip outer, inner;
	struct icmphdr icmp;
	struct udphdr udp;
	size_t off = 0;

	ft_bzero(&outer, sizeof(outer));
	ft_bzero(&inner, sizeof(inner));
	ft_bzero(&icmp, sizeof(icmp));
	ft_bzero(&udp, sizeof(udp));
	outer.ip_hl = 5;
	outer.ip_p = IPPROTO_ICMP;
	icmp.type = (uint8_t)type;
	icmp.code = (uint8_t)code;
	inner.ip_hl = 5;
	inner.ip_p = IPPROTO_UDP;
	inet_aton(dst, &inner.ip_dst);
	udp.uh_sport = htons(sport);
	udp.uh_dport = htons(dport);
	ft_memcpy(buf + off, &outer, sizeof(outer));
	off += sizeof(outer);
	ft_memcpy(buf + off, &icmp, sizeof(icmp));
	off += sizeof(icmp);
	ft_memcpy(buf + off, &inner, sizeof(inner));
	off += sizeof(inner);
	ft_memcpy(buf + off, &udp, sizeof(udp));
	return off + sizeof(udp);
}

static void setup(t_traceroute *tr) {
	ft_bzero(tr, sizeof(*tr));
	tr->src_port = OUR_SPORT;
	inet_aton("10.0.0.9", &tr->dest_addr.sin_addr);
}

static void test_accepts_time_exceeded(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_TIME_EXCEEDED, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	CHECK(parse_icmp_reply(buf, len, &tr, &reply));
	CHECK_INT(reply.type, ICMP_TIME_EXCEEDED);
	CHECK_INT(reply.code, 0);
	CHECK_INT(reply.dport, OUR_DPORT);
}

static void test_accepts_port_unreachable(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	CHECK(parse_icmp_reply(buf, len, &tr, &reply));
	CHECK_INT(reply.type, ICMP_DEST_UNREACH);
	CHECK_INT(reply.code, ICMP_PORT_UNREACH);
}

static void test_ignores_other_programs(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_TIME_EXCEEDED, 0, OUR_SPORT + 1, OUR_DPORT, "10.0.0.9");
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
	len = build(buf, ICMP_TIME_EXCEEDED, 0, OUR_SPORT, OUR_DPORT, "10.0.0.8");
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
}

static void test_ignores_other_icmp_types(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_ECHOREPLY, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
	len = build(buf, ICMP_ECHO, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
}

static void test_ignores_non_udp_probe(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_TIME_EXCEEDED, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	buf[sizeof(struct ip) + sizeof(struct icmphdr) + 9] = IPPROTO_TCP; /* inner ip_p */
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
}

/* Every shorter prefix of a valid packet must be rejected without reading past `len`.
 * Run under the sanitizer build this also proves there is no out-of-bounds read. */
static void test_truncated_packets(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char full[128];
	unsigned char *copy;
	size_t len, cut;

	setup(&tr);
	len = build(full, ICMP_TIME_EXCEEDED, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	for (cut = 0; cut < len; cut++) {
		copy = malloc(cut ? cut : 1);
		ft_memcpy(copy, full, cut);
		CHECK(!parse_icmp_reply(copy, cut, &tr, &reply));
		free(copy);
	}
}

static void test_bogus_header_lengths(void) {
	t_traceroute tr;
	t_reply reply;
	unsigned char buf[128];
	size_t len;

	setup(&tr);
	len = build(buf, ICMP_TIME_EXCEEDED, 0, OUR_SPORT, OUR_DPORT, "10.0.0.9");
	buf[0] = 0x44; /* outer header claims 16 bytes: shorter than a real IP header */
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
	buf[0] = 0x4f; /* outer header claims 60 bytes: runs past the packet */
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
	buf[0] = 0x45;
	buf[sizeof(struct ip) + sizeof(struct icmphdr)] = 0x4f; /* same for the quoted header */
	CHECK(!parse_icmp_reply(buf, len, &tr, &reply));
}

static void test_markers(void) {
	char buf[16];

	format_marker(ICMP_TIME_EXCEEDED, 0, buf, sizeof(buf));
	CHECK_STR(buf, "");
	format_marker(ICMP_DEST_UNREACH, ICMP_PORT_UNREACH, buf, sizeof(buf));
	CHECK_STR(buf, "");
	format_marker(ICMP_DEST_UNREACH, ICMP_NET_UNREACH, buf, sizeof(buf));
	CHECK_STR(buf, " !N");
	format_marker(ICMP_DEST_UNREACH, ICMP_HOST_UNREACH, buf, sizeof(buf));
	CHECK_STR(buf, " !H");
	format_marker(ICMP_DEST_UNREACH, ICMP_PROT_UNREACH, buf, sizeof(buf));
	CHECK_STR(buf, " !P");
	format_marker(ICMP_DEST_UNREACH, ICMP_FRAG_NEEDED, buf, sizeof(buf));
	CHECK_STR(buf, " !F");
	format_marker(ICMP_DEST_UNREACH, ICMP_PKT_FILTERED, buf, sizeof(buf));
	CHECK_STR(buf, " !X");
	format_marker(ICMP_DEST_UNREACH, 99, buf, sizeof(buf));
	CHECK_STR(buf, " !<99>");
}

int main(void) {
	RUN(test_accepts_time_exceeded);
	RUN(test_accepts_port_unreachable);
	RUN(test_ignores_other_programs);
	RUN(test_ignores_other_icmp_types);
	RUN(test_ignores_non_udp_probe);
	RUN(test_truncated_packets);
	RUN(test_bogus_header_lengths);
	RUN(test_markers);

	printf("\n%d checks, %d failed\n", g_tests_run, g_tests_failed);
	return g_tests_failed != 0;
}
