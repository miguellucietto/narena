CC = cc
CFLAGS = -Wall -Wextra -Werror -ggdb

TEST_SOURCES := $(wildcard tests/*.c)
TEST_BINS := $(patsubst tests/%.c, build/tests/%, $(TEST_SOURCES))

.PHONY: tests clean

tests: $(TEST_BINS)

build/tests/%: tests/%.c narena.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf build
