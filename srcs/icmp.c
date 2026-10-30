#include "ft_traceroute.h"

/* Reads an ICMP error packet received on the raw socket (outer IP header included) and
 * checks, before touching any byte, that everything we read fits inside `len`.
 * The packet is only accepted when the probe it quotes is one of ours: a UDP packet
 * sent from our own source port to the traced destination. */
bool parse_icmp_reply(const unsigned char *buf, size_t len, const t_traceroute *traceroute,
                      t_reply *reply) {
	struct ip outer, inner;
	struct icmphdr icmp;
	struct udphdr udp;
	size_t outer_len, inner_len, inner_off;

	if (len < sizeof(outer))
		return false;
	ft_memcpy(&outer, buf, sizeof(outer));
	outer_len = (size_t)outer.ip_hl << 2;
	if (outer_len < sizeof(outer) || len < outer_len + sizeof(icmp))
		return false;
	ft_memcpy(&icmp, buf + outer_len, sizeof(icmp));
	if (icmp.type != ICMP_TIME_EXCEEDED && icmp.type != ICMP_DEST_UNREACH)
		return false;

	inner_off = outer_len + sizeof(icmp);
	if (len < inner_off + sizeof(inner))
		return false;
	ft_memcpy(&inner, buf + inner_off, sizeof(inner));
	inner_len = (size_t)inner.ip_hl << 2;
	if (inner_len < sizeof(inner) || inner.ip_p != IPPROTO_UDP ||
	    len < inner_off + inner_len + sizeof(udp))
		return false;
	if (inner.ip_dst.s_addr != traceroute->dest_addr.sin_addr.s_addr)
		return false;
	ft_memcpy(&udp, buf + inner_off + inner_len, sizeof(udp));
	if (ntohs(udp.uh_sport) != traceroute->src_port)
		return false;

	reply->type = icmp.type;
	reply->code = icmp.code;
	reply->dport = ntohs(udp.uh_dport);
	return true;
}

/* Writes the marker real traceroute prints after the RTT of an ICMP "unreachable"
 * (" !H", " !N", ...). Port unreachable is how the destination answers: no marker. */
void format_marker(int type, int code, char *buf, size_t size) {
	static const char letters[] = "NHP.FSUWIAZQTXVC";

	buf[0] = '\0';
	if (type != ICMP_DEST_UNREACH || code == ICMP_PORT_UNREACH)
		return;
	if (code >= 0 && code < (int)sizeof(letters) - 1)
		snprintf(buf, size, " !%c", letters[code]);
	else
		snprintf(buf, size, " !<%d>", code);
}
