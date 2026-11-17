CFLAGS=-Wall -Wextra -std=c99 -O0
SRC ?= pe_src.c
test: test_pe_src
	./test_pe_src
test_pe_src: $(SRC) test_pe_src.c pe_src.h
	$(CC) $(CFLAGS) -I. -o $@ $(SRC) test_pe_src.c
# 驗證參考解答：make check-solution
check-solution:
	$(MAKE) clean && $(MAKE) test SRC=solution/pe_src.c
clean:
	rm -f test_pe_src
