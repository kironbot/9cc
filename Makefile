CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
		$(CC) -g -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
		./9cc tests > tmp.s
		gcc -static -o tmp tmp.s
		./tmp

self: 9cc
		./9cc 9cc.h tokenize.c type.c parse.c codegen.c main.c | grep -v "^;" > tmp.s
		gcc -g -static -o 9cc_self tmp.s
		./9cc_self 9cc.h tokenize.c type.c parse.c codegen.c main.c | grep -v "^;" > tmp2.s
		diff -ur tmp.s tmp2.s

clean:
		rm -f 9cc *.o *~ tmp*

.PHONY: test clean