CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude -Ithird_party/unity -Ithird_party/sha256
SRC     = $(filter-out src/main.c, $(wildcard src/*.c)) $(wildcard third_party/sha256/*.c)
TESTS   = $(wildcard tests/test_*.c)
UNITY   = third_party/unity/unity.c

ifeq ($(OS),Windows_NT)
    SHELL := cmd.exe
    MKDIR_BUILD = if not exist build mkdir build
    CLEAN_CMD = if exist build rmdir /s /q build & if exist app.exe del /q app.exe
    RUN_TESTS = for %%t in ($(subst /,\,$^)) do ( echo == %%t == & %%t )
else
    MKDIR_BUILD = mkdir -p build
    CLEAN_CMD = rm -rf build app app.exe
    RUN_TESTS = for t in $^; do echo "== $$t =="; ./$$t || exit 1; done
endif

all: app

app: $(SRC) src/main.c
	$(CC) $(CFLAGS) -o app $^

test: $(TESTS:tests/%.c=build/%)
	@$(RUN_TESTS)

build/%: tests/%.c $(SRC) $(UNITY)
	@$(MKDIR_BUILD)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@$(CLEAN_CMD)