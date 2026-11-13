CFLAGS=-Wall -Wextra -std=c99 -O0
test: test_pd_codec
	./test_pd_codec
test_pd_codec: pd_codec.c test_pd_codec.c pd_codec.h
	$(CC) $(CFLAGS) -o $@ pd_codec.c test_pd_codec.c
clean:
	rm -f test_pd_codec
