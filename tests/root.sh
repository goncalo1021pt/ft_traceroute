#!/bin/sh
# Real traces against 127.0.0.1. Raw sockets need CAP_NET_RAW:
#   - as root (CI):        sh tests/root.sh ./ft_traceroute
#   - as a normal user:    the script re-runs itself inside a user+network namespace,
#                          where you are root and have your own private loopback.
# usage: sh tests/root.sh [path/to/binary]

BIN=${1:-./ft_traceroute}

# Everything runs in private user/mount/network namespaces, so the tests can create
# interfaces and routes without touching the machine: unshare -n when already root (CI),
# unshare -rn otherwise (you are root only inside the namespace).
if [ -z "$FT_IN_NS" ]; then
	if [ "$(id -u)" -eq 0 ]; then
		exec unshare -nm env FT_IN_NS=1 sh "$0" "$BIN"
	fi
	exec unshare -rnm env FT_IN_NS=1 sh "$0" "$BIN"
fi
ip link set lo up

PASSED=0
FAILED=0

pass() {
	PASSED=$((PASSED + 1))
}

fail() {
	FAILED=$((FAILED + 1))
	printf 'FAIL %s\n' "$1"
	[ -n "$2" ] && printf '  %s\n' "$2"
}

# number of RTT values ("1.234 ms") printed in the given text
count_rtts() {
	printf '%s\n' "$1" | grep -o ' ms' | wc -l
}

# a plain localhost trace: header, then a single hop that reaches the destination
out=$("$BIN" 127.0.0.1 2>&1)
code=$?
[ "$code" -eq 0 ] && pass || fail "localhost trace exit code" "got $code"
printf '%s\n' "$out" | head -n 1 | grep -qF 'ft_traceroute to 127.0.0.1 (127.0.0.1), 30 hops max, 60 byte packets' &&
	pass || fail "header line" "$(printf '%s\n' "$out" | head -n 1)"
[ "$(printf '%s\n' "$out" | wc -l)" -eq 2 ] && pass || fail "stops at the first hop" "$out"
[ "$(count_rtts "$out")" -eq 3 ] && pass || fail "default is 3 probes per hop" "$out"
printf '%s\n' "$out" | sed -n 2p | grep -qE '^ 1  .*\(127\.0\.0\.1\)' &&
	pass || fail "hop line format" "$(printf '%s\n' "$out" | sed -n 2p)"

# output must match real traceroute's layout: no trailing whitespace anywhere
printf '%s\n' "$out" | grep -q '[[:space:]]$' && fail "no trailing whitespace" "$(printf '%s\n' "$out" | cat -A)" || pass
printf '%s\n' "$out" | sed -n 2p | grep -qE '^ 1  [^ ]+ \(127\.0\.0\.1\)  [0-9.]+ ms  [0-9.]+ ms  [0-9.]+ ms$' &&
	pass || fail "exact hop line layout" "$(printf '%s\n' "$out" | sed -n 2p | cat -A)"

# -q controls how many probes are sent per hop, including both limits
for q in 1 2 5 10; do
	out=$("$BIN" -q "$q" 127.0.0.1 2>&1)
	[ "$(count_rtts "$out")" -eq "$q" ] && pass || fail "-q $q prints $q RTTs" "$out"
done
out=$("$BIN" --queries=4 127.0.0.1 2>&1)
[ "$(count_rtts "$out")" -eq 4 ] && pass || fail "--queries=4 prints 4 RTTs" "$out"

# -m changes the header
out=$("$BIN" -m 5 127.0.0.1 2>&1)
printf '%s\n' "$out" | head -n 1 | grep -qF '5 hops max' && pass || fail "-m 5 shows 5 hops max" "$out"

# a hostname that resolves to loopback
out=$("$BIN" localhost 2>&1)
printf '%s\n' "$out" | head -n 1 | grep -qF 'ft_traceroute to localhost (127.0.0.1)' &&
	pass || fail "trace by hostname" "$(printf '%s\n' "$out" | head -n 1)"

# Hops that never answer print only stars (10.9.9.0/24 is routed to lo, where nothing replies).
# All probes of the three hops wait together, so this takes one timeout (5 s), not nine.
ip route add 10.9.9.0/24 dev lo 2>/dev/null
start=$(date +%s)
out=$("$BIN" -m 3 10.9.9.9 2>&1)
took=$(($(date +%s) - start))
[ "$(printf '%s\n' "$out" | sed -n 2p)" = " 1  * * *" ] && [ "$(printf '%s\n' "$out" | sed -n 4p)" = " 3  * * *" ] &&
	pass || fail "silent hops print only stars" "$(printf '%s\n' "$out" | cat -A)"
[ "$took" -ge 5 ] && [ "$took" -le 7 ] && pass || fail "silent hops share one timeout (took ${took}s, expected 5-7)"

# Three machines in a row:  us -- router (10.1.0.2 / 10.2.0.1) -- destination (10.2.0.2)
# This is the only way to see TTL-exceeded replies, markers and several hops locally.
# The kernel rate-limits ICMP errors; switch that off so -q 10 gets ten answers.
if mount -t tmpfs none /run 2>/dev/null && mkdir -p /run/netns && ip netns add r && ip netns add d; then
	ip link add v1 type veth peer name v1r && ip link set v1r netns r
	ip link add v2 type veth peer name v2d && ip link set v2 netns r && ip link set v2d netns d
	ip addr add 10.1.0.1/24 dev v1 && ip link set v1 up
	ip route add 10.2.0.0/24 via 10.1.0.2
	ip route add 10.3.0.0/24 via 10.1.0.2
	sysctl -qw net.ipv4.icmp_ratelimit=0 net.ipv4.icmp_ratemask=0
	ip netns exec r sh -c 'ip link set lo up; ip addr add 10.1.0.2/24 dev v1r; ip link set v1r up
		ip addr add 10.2.0.1/24 dev v2; ip link set v2 up
		sysctl -qw net.ipv4.ip_forward=1 net.ipv4.icmp_ratelimit=0 net.ipv4.icmp_ratemask=0
		ip route add unreachable 10.3.0.0/24'
	ip netns exec d sh -c 'ip link set lo up; ip addr add 10.2.0.2/24 dev v2d; ip link set v2d up
		ip route add 10.1.0.0/24 via 10.2.0.1; sysctl -qw net.ipv4.icmp_ratelimit=0 net.ipv4.icmp_ratemask=0'

	out=$("$BIN" 10.2.0.2 2>&1)
	[ "$(printf '%s\n' "$out" | wc -l)" -eq 3 ] && pass || fail "two-hop trace has header + 2 hops" "$out"
	printf '%s\n' "$out" | sed -n 2p | grep -qE '^ 1  [^ ]+ \(10\.1\.0\.2\)  [0-9.]+ ms  [0-9.]+ ms  [0-9.]+ ms$' &&
		pass || fail "hop 1 is the router" "$(printf '%s\n' "$out" | sed -n 2p)"
	printf '%s\n' "$out" | sed -n 3p | grep -qE '^ 2  [^ ]+ \(10\.2\.0\.2\)  [0-9.]+ ms  [0-9.]+ ms  [0-9.]+ ms$' &&
		pass || fail "hop 2 is the destination" "$(printf '%s\n' "$out" | sed -n 3p)"

	out=$("$BIN" -q 10 10.2.0.2 2>&1)
	[ "$(count_rtts "$(printf '%s\n' "$out" | sed -n 2p)")" -eq 10 ] &&
		[ "$(count_rtts "$(printf '%s\n' "$out" | sed -n 3p)")" -eq 10 ] &&
		pass || fail "-q 10 gets ten answers on every hop" "$out"

	# The router answers "host unreachable" for 10.3.0.0/24. Depending on the kernel it does so
	# at the first or at the second hop (route lookup before or after the TTL check), so only
	# check that the trace ends there, on the router's line, with the marker.
	# -N only changes how many probes are in flight, never what is printed
	ref=$("$BIN" 10.2.0.2 2>&1 | sed 's/[0-9.]* ms/X ms/g')
	for n in 1 2 5; do
		out=$("$BIN" -N "$n" 10.2.0.2 2>&1 | sed 's/[0-9.]* ms/X ms/g')
		[ "$out" = "$ref" ] && pass || fail "-N $n prints the same hops as the default" "$out"
	done

	# The kernel still drops some ICMP errors for unroutable destinations, and real traceroute
	# loses the same replies, so a lost answer gets a few more tries before it counts as a failure.
	for attempt in 1 2 3 4; do
		out=$("$BIN" 10.3.0.1 2>&1)
		last=$(printf '%s\n' "$out" | tail -n 1)
		if printf '%s\n' "$last" | grep -qE '^ [12]  .*\(10\.1\.0\.2\)  [0-9.]+ ms !H' &&
			[ "$(printf '%s\n' "$out" | wc -l)" -le 3 ]; then
			break
		fi
		sleep 1
	done
	printf '%s\n' "$last" | grep -qE '^ [12]  .*\(10\.1\.0\.2\)  [0-9.]+ ms !H' &&
		pass || fail "unreachable hop shows !H" "$out"
	[ "$(printf '%s\n' "$out" | wc -l)" -le 3 ] && pass || fail "trace stops at the unreachable hop" "$out"
else
	fail "could not build the three-machine test network"
fi

# no route to the destination: one error, exit 2, no 30 lines of stars
out=$("$BIN" 10.77.0.1 2>&1)
code=$?
[ "$code" -eq 2 ] && [ "$(printf '%s\n' "$out" | grep -c 'Network is unreachable')" -eq 1 ] &&
	pass || fail "no route fails once with exit 2" "exit $code: $out"

# string options must not leak or crash (-i/-s/-t/-l are only stored, not used)
"$BIN" -i lo -i lo -s 127.0.0.1 -t 1 -l 2 -q 1 127.0.0.1 >/dev/null 2>&1
[ $? -eq 0 ] && pass || fail "string options are accepted"

# valgrind: no leaks, no invalid accesses, no leaked descriptors (skipped when absent or sanitized)
if command -v valgrind >/dev/null 2>&1 && ! nm "$BIN" 2>/dev/null | grep -q __asan_init; then
	valgrind --error-exitcode=99 --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all \
		"$BIN" -i lo -s 127.0.0.1 -q 2 127.0.0.1 >/dev/null 2>vg.log
	if [ $? -eq 99 ]; then
		fail "valgrind is clean" "see $(pwd)/vg.log"
	else
		pass
		rm -f vg.log
	fi
fi

printf 'root: %d passed, %d failed\n' "$PASSED" "$FAILED"
[ "$FAILED" -eq 0 ]
