CC ?= cc
CFLAGS ?= -std=gnu11 -Wall -Wextra -Wno-unused-parameter
DIRECTORY ?= src
PATTERN ?= .c
THREADS ?= 4

.PHONY: all build run test check clean
all: build
build: build/pfind

build/pfind: src/pfind.c
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -pthread -o $@ src/pfind.c

run: build
	./build/pfind "$(DIRECTORY)" "$(PATTERN)" "$(THREADS)"

test: build
	uv run --no-project python tests/test_pfind.py build/pfind

check: test

clean:
	rm -rf build
