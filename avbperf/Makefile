
# install prefix either /usr or /usr/local on most unix systems
PREFIX ?= /usr

#avnu avtp
AVB_CFLAGS := 
AVB_LDLIBS := avtp


CFLAGS  += $(AVB_CFLAGS)
LDLIBS  += $(AVB_LDLIBS)
LDFLAGS := -l

all: avbperf


%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $+


avbperf: avbperf.o common.o
	$(CC) -o $@ $+ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o avbperf
