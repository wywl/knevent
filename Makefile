CC = gcc
CFLAGS = -Wall -I./include/
OBJS = src/knBaseUtil.o

all: testutil
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 
testutil: $(OBJS) test/test.o
	$(CC) -o $@ test/test.o $(OBJS) -lm

clean:
	rm -f testutil *.o
