CFLAGS=-Wall -Wextra -std=c99 -O0
SRC ?= stack_guard.c
test: test_stack_guard
	./test_stack_guard
test_stack_guard: $(SRC) test_stack_guard.c stack_guard.h
	$(CC) $(CFLAGS) -I. -o $@ $(SRC) test_stack_guard.c
# 驗證參考解答：make check-solution
check-solution:
	$(MAKE) clean && $(MAKE) test SRC=solution/stack_guard.c
clean:
	rm -f test_stack_guard
