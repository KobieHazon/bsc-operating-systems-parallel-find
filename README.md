# BSc Operating Systems - Parallel Find

- Course: Operating Systems.

My multithreaded filename-search program. Worker threads recursively scan a directory using a shared queue and print files whose names contain a given substring. It searches **names**, not file contents; matching is case-sensitive.

## Requirements

macOS or Linux, a C compiler, POSIX threads, and Make. On macOS, run `xcode-select --install` if needed; on Debian/Ubuntu, install `build-essential`. The optional test suite also uses Python 3 through `uv`.

## Build and run

From the repository root:

```sh
make build
make run
```

The default demonstration searches `src` for `.c` with four worker threads:

```text
src/pfind.c
Done searching, found 1 files
```

The executable accepts exactly three arguments:

```sh
./build/pfind DIRECTORY FILENAME_SUBSTRING THREAD_COUNT
./build/pfind . README 4
# Quote a directory or search term containing spaces:
./build/pfind "./my files" "report" 2
```

The directory must exist and the thread count must be a positive integer. For the Make shortcut:

```sh
make run DIRECTORY=. PATTERN=README THREADS=4
```

Matching paths are followed by a total. Output order may vary because workers run concurrently. No match is a successful search with zero results. Symbolic links are skipped, so linked directories cannot create recursive cycles. The program reads directory metadata and does not modify the searched files.

## Tests

```sh
make test
```

The tests run actual searches with one and multiple workers, nested directories, filenames containing spaces, empty/no-match cases, a symbolic-link cycle, and invalid arguments. `make check` runs the same tests; `make clean` removes build output.

## Files

- `src/pfind.c`: the search program.
- `tests/test_pfind.py`: temporary-directory runtime tests.
