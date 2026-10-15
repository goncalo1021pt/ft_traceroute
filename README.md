# ft_traceroute
A implementation of the traceroute unix command

## Development

| Command | What it does |
|---|---|
| `make test` | unit tests (parser) and command-line tests, no root needed |
| `make root-tests` | real traces against `127.0.0.1` plus a valgrind run; re-runs itself in a user/network namespace if you are not root |
| `make asan` / `make root-tests-asan` | build and test with AddressSanitizer + UBSan |
| `make warnings` | compile with extra-strict warnings |
| `make format` / `make format-check` | apply / verify the `.clang-format` style (needs `clang-format`) |
| `make lint` | static analysis (needs `cppcheck`) |

CI (`.github/workflows/ci.yml`) runs all of these on every push and pull request to `main`.
