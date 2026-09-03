# BSc Operating Systems - Parallel Find

A historical archive of my CS BSc coursework.

## Contents

Multithreaded recursive file search coursework using pthreads, a shared directory queue, condition variables, and SIGINT handling.

## Provenance

- Era: CS BSc.
- Last recovered work: 2019-2020 archive copy.
- Supplied exercise material is identified separately below.
- My implementation is kept separately from supplied exercise files.
- Submitted ZIP wrappers and Apple resource forks were omitted.

## Files

Exercise/framework material:

- `assignment/MISSING_HANDOUT.md`

Implementation material:

- `src/pfind.c`

## Tech Stack

- C.
- POSIX APIs where applicable.
- `pthread` for the parallel-find assignment.
- Linux kernel-module APIs for the message-slot assignment.

## Validate

```bash
make check
```

## Notes

The matching original handout was not recovered; later parallel-threading examples are reference evidence only and are not a substitute for the matching handout.
