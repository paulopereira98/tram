
# install prefix either /usr or /usr/local on most unix systems
PREFIX ?= /usr

#avnu avtp
AVB_CFLAGS := 
AVB_LDLIBS := -lavtp

#avnu avtp
ALSA_CFLAGS := 
ALSA_LDLIBS := -lasound


CFLAGS  += $(AVB_CFLAGS) $(ALSA_CFLAGS)
LDLIBS  += $(AVB_LDLIBS) $(ALSA_LDLIBS)
LDFLAGS := -lm

all: avbperf


%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $+


avbperf: avbperf.o common.o avb_aaf.o avb_alsa.o
	$(CC) -o $@ $+ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o avbperf
