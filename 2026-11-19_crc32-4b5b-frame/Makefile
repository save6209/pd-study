CFLAGS=-Wall -Wextra -std=c99 -O0
SRC ?= pd_phy.c
test: test_pd_phy
	./test_pd_phy
test_pd_phy: $(SRC) test_pd_phy.c pd_phy.h
	$(CC) $(CFLAGS) -I. -o $@ $(SRC) test_pd_phy.c
# 驗證參考解答：make check-solution
check-solution:
	$(MAKE) clean && $(MAKE) test SRC=solution/pd_phy.c
clean:
	rm -f test_pd_phy
