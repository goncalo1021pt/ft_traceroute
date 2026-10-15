#!/bin/sh
# Command-line tests that do not need root: they all stop before a raw socket is opened.
# usage: sh tests/cli.sh [path/to/binary]

BIN=${1:-./ft_traceroute}
PASSED=0
FAILED=0

# expect <name> <exit code> <text that must appear in stdout+stderr> <args...>
expect() {
	name=$1
	want_code=$2
	want_text=$3
	shift 3
	out=$("$BIN" "$@" 2>&1)
	code=$?
	if [ "$code" -eq "$want_code" ] && printf '%s' "$out" | grep -qF -- "$want_text"; then
		PASSED=$((PASSED + 1))
	else
		FAILED=$((FAILED + 1))
		printf 'FAIL %s\n  args:     %s\n  exit:     %s (want %s)\n  want:     %s\n  output:   %s\n' \
			"$name" "$*" "$code" "$want_code" "$want_text" "$out"
	fi
}

expect "no arguments"          1 "Destination address required"
expect "help"                  0 "Usage: ft_traceroute" --help
expect "help lists -q"         0 "--queries" --help
expect "help ignores the rest" 0 "Usage: ft_traceroute" --help 127.0.0.1
expect "missing destination"   1 "Destination address required" -q 2
expect "unknown option"        1 "usage error" -Z 127.0.0.1
expect "queries 0"             1 "no more than 10 probes per hop" -q 0 127.0.0.1
expect "queries 11"            1 "no more than 10 probes per hop" -q 11 127.0.0.1
expect "queries negative"      1 "no more than 10 probes per hop" -q -1 127.0.0.1
expect "queries not a number"  1 "no more than 10 probes per hop" -q abc 127.0.0.1
expect "queries long option"   1 "no more than 10 probes per hop" --queries=11 127.0.0.1
expect "unknown host"          255 "Name or service not known" nonexistent.invalid

# Without root the raw ICMP socket cannot be opened, which is itself worth checking.
if [ "$(id -u)" -ne 0 ]; then
	expect "needs root" 255 "Operation not permitted" 127.0.0.1
fi

printf 'cli: %d passed, %d failed\n' "$PASSED" "$FAILED"
[ "$FAILED" -eq 0 ]
