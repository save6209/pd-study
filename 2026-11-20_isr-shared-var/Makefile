CFLAGS=-Wall -Wextra -std=c99 -O0
SRC ?= tick.c
test: test_tick
	./test_tick
test_tick: $(SRC) sim.c test_tick.c tick.h sim.h
	$(CC) $(CFLAGS) -I. -o $@ $(SRC) sim.c test_tick.c
# 驗證參考解答：make check-solution
check-solution:
	$(MAKE) clean && $(MAKE) test SRC=solution/tick.c
clean:
	rm -f test_tick
