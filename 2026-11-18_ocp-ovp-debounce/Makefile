CFLAGS=-Wall -Wextra -std=c99 -O0
SRC ?= prot.c
test: test_prot
	./test_prot
test_prot: $(SRC) test_prot.c prot.h
	$(CC) $(CFLAGS) -I. -o $@ $(SRC) test_prot.c
# 驗證參考解答：make check-solution
check-solution:
	$(MAKE) clean && $(MAKE) test SRC=solution/prot.c
clean:
	rm -f test_prot
