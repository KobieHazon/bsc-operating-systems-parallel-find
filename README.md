# BSc Operating Systems - Parallel Find

- Course: BSc Computer Science.
- Available copy: 2019–2020.
- Supplied exercise material is identified separately below.
- My implementation is kept separately from supplied exercise files.
- Submitted ZIP wrappers and Apple resource forks were omitted.

## Contents

Multithreaded recursive file search coursework using pthreads, a shared directory queue, condition variables, and SIGINT handling.

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
