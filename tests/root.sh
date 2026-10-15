#!/bin/sh
# Real traces against 127.0.0.1. Raw sockets need CAP_NET_RAW:
#   - as root (CI):        sh tests/root.sh ./ft_traceroute
#   - as a normal user:    the script re-runs itself inside a user+network namespace,
#                          where you are root and have your own private loopback.
# usage: sh tests/root.sh [path/to/binary]

BIN=${1:-./ft_traceroute}

if [ "$(id -u)" -ne 0 ]; then
	exec unshare -rn sh -c 'ip link set lo up && exec sh "$0" "$@"' "$0" "$BIN"
fi

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

# a hop that never answers prints only stars (10.9.9.0/24 is routed to lo, where nothing replies)
ip route add 10.9.9.0/24 dev lo 2>/dev/null
out=$("$BIN" -m 1 -q 1 10.9.9.9 2>&1)
[ "$(printf '%s\n' "$out" | sed -n 2p)" = " 1  *" ] && pass || fail "silent hop prints only a star" "$(printf '%s\n' "$out" | cat -A)"

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
