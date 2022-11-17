
# install prefix either /usr or /usr/local on most unix systems
PREFIX ?= /usr

CC := gcc

LD_LIBRARY_PATH += /usr/local/lib

# avnu avtp
AVB_CFLAGS :=
AVB_LDLIBS := -lm -lavtp

# ALSA
ALSA_CFLAGS := 
ALSA_LDLIBS := -lasound

# WAV library
WAV_CFLAGS := -Wno-multichar -Wno-format-truncation
WAV_LDLIBS := #-lwav

CFLAGS  += $(AVB_CFLAGS) $(ALSA_CFLAGS)
LDLIBS  += $(AVB_LDLIBS) $(ALSA_LDLIBS)
LDFLAGS := -W -Wall

all: avbperf


%.o: %.c
	$(CC) -o $@ -c $(CFLAGS) $+

wav.o: wav.c
	$(CC) -o $@ -c $(WAV_CFLAGS) $+

avbperf: avbperf.o common.o avb_aaf.o avb_alsa.o wav.o
	$(CC) -o $@ $+ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o avbperf
