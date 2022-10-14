CC=clang

all:
	#$(CC) test.c -fsanitize=address -o test
	$(CC) -fsanitize=address  test.c -c -o test.o
	$(CC) -fsanitize=address  test.o -o test

clean:
	rm -f test
	rm -f test.o
