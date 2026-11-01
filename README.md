# ft_traceroute

*This project has been created as part of the 42 curriculum by goncalo1021pt.*

## Description

A re-implementation of the Unix `traceroute` command in C for IPv4. It sends UDP probes with
an increasing TTL and reads the ICMP "time exceeded" / "unreachable" answers from a raw socket
to show every router on the way to a destination, with the round-trip time of each probe.
The output follows the real `traceroute`:

```
ft_traceroute to 10.2.0.2 (10.2.0.2), 30 hops max, 60 byte packets
 1  10.1.0.2 (10.1.0.2)  0.153 ms  0.101 ms  0.092 ms
 2  10.2.0.2 (10.2.0.2)  0.201 ms  0.148 ms  0.139 ms
```

How it works:

- Probe `i` goes to UDP port `port + i` from one bound source port. A reply is accepted only if
  the ICMP packet quotes that destination port, our source port and our destination address, so
  other traffic on the raw socket is ignored. Lengths are checked before any header is read.
- Several probes are in flight at once (`-N`, default 16), each with its own timeout, and hops
  are printed in order when complete. A trace of silent hops costs one timeout, not one per probe.
- RTTs use the kernel receive timestamp (`SO_TIMESTAMP`), so printing or name lookups never
  inflate them.
- The trace ends at the hop where the destination answers "port unreachable". Markers such as
  `!H`, `!N`, `!X` are printed after the RTT of other unreachable answers, as real `traceroute` does.

## Instructions

Requirements: a C compiler, `make`, Linux. Raw sockets need root (or `CAP_NET_RAW`).

```
make              # mandatory part
make bonus        # mandatory part + the bonuses below
sudo ./ft_traceroute [options] <host>
./ft_traceroute --help
```

`make` and `make bonus` use separate object directories and the binary is relinked when you
switch between them.

### Mandatory part

`-m/--max-hops`, `-p/--port`, `-q/--queries` (1 to 10), `-N/--sim-queries`, `--help`.
The mandatory binary never resolves hop names: hops print as `ip (ip)`, and the lookup code is
not even compiled in (`nm ft_traceroute | grep getnameinfo` finds nothing).

### Bonus part (`make bonus`)

| Option | Effect |
|---|---|
| (default) | reverse DNS of every hop: `router.lan (10.0.0.1)` |
| `-n` | do not resolve names, print the address only |
| `-f, --first-ttl` | start at this hop instead of 1 |
| `-w, --wait` | seconds to wait for each reply (1 to 60, default 5) |
| `-t, --tos` | IP type of service, 0 to 255 |
| `-s, --source` | source address of the probes |
| `-i, --interface` | send the probes through this interface |

Exit codes: 0 on success, 1 for a usage error, 2 for resolve, socket or send errors (like `traceroute`).

## Development

| Command | What it does |
|---|---|
| `make test` | unit tests (parser, ICMP validation) and command-line tests, no root needed |
| `make root-tests` | real traces against `127.0.0.1` and a three-machine network of namespaces, plus a valgrind run; re-runs itself in a user/network namespace if you are not root |
| `make unit-asan` / `make root-tests-asan` | the same tests with AddressSanitizer + UBSan |
| `make warnings` | compile with extra-strict warnings |
| `make format` / `make format-check` | apply / verify the `.clang-format` style (needs `clang-format`) |
| `make lint` | static analysis (needs `cppcheck`) |

Add `MODE=bonus` to any of them to run it against the bonus build. CI
(`.github/workflows/ci.yml`) runs everything for both builds on every pull request to `main`.

## Resources

- `man 7 raw`, `man 7 icmp`, `man 7 ip`, `man 2 recvmsg`, `man 7 socket` (`SO_TIMESTAMP`)
- RFC 792 (ICMP) and RFC 791 (IP)
- The behaviour and output of the system `traceroute`, used as the reference to compare against

## Use of AI

AI (Claude Code) was used as a coding partner on this project. Be clear about what it did:

- **Generated with AI, then reviewed and run by me:** the reply matching and ICMP validation,
  the parallel probe engine, the DNS/bonus build split, the bonus options (`-f -w -t -s -i`),
  the unit, command-line and namespace-based tests, the CI workflow and formatter configuration.
- **AI-assisted review:** a review of the first version found the problems listed in the commit
  history (RTTs inflated by DNS, wrong exit codes, unchecked packet lengths, quadratic waiting on
  silent hops), which were then fixed in separate pull requests.
- **My own decisions:** the scope and order of the work (one group of changes per pull request),
  the code style, the mandatory/bonus split, and what to keep or remove (for example dropping `-l`).

Everything was run and tested by me, and I can explain and defend every part of it.
