CC=gcc
CFLAGS=-I $(KLEE_INCLUDE_PATH)

DEP = runtime.h
SRC = runtime.c restore.c
OBJ_KLEE = $(SRC:%.c=%.klee.o)
OBJ_TEST = $(SRC:%.c=%.test.o)

all: libangelix.bc dg_record.o   #libangelix.test.a libangelix.klee.a

dg_record.o:
	gcc -g record.c -c 
	gcc -g ht_table.c -c
	gcc -g restore.c -c 
	ld -r record.o ht_table.o restore.o -o $@

libangelix.bc: $(SRC) $(DEP)
	clang-11 -g -O0 -emit-llvm -c -o $@ $< $(CFLAGS)

%.test.o: %.c $(DEP)
	clang-11 -g -c -o $@ $< $(CFLAGS)

%.klee.o: %.c $(DEP)
	clang-11 -g -O0 -emit-llvm -c -o $@ $< $(CFLAGS)

libangelix.klee.a: $(OBJ_KLEE)
	llvm-ar rcs $@ $(OBJ_KLEE)

libangelix.test.a: $(OBJ_TEST)
	ar rcs $@ $(OBJ_TEST)

clean:
	rm -f *.o
	rm -f *.a
