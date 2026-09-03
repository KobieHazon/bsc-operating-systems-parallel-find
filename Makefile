check:
	python3 scripts/check_repository.py
	mkdir -p build
	cc -std=gnu11 -Wall -Wextra -Wno-unused-parameter -pthread -o build/pfind src/pfind.c
	./build/pfind . README 1

clean:
	rm -rf build
