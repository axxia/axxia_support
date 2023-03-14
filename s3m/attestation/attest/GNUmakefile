################################################################################
# GNUmakefile
#
# Makefile for the simple oobmsm CPU attestation example
################################################################################

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Wmissing-prototypes -Wstrict-prototypes -O -ggdb -g3

ifdef SYSROOT
CFLAGS += --sysroot=$(SYSROOT)
endif

ISGIT := $(shell git ls-files 1>/dev/null 2>&1 ; echo $$?)

ifeq ($(ISGIT),0)
	VER="$(shell git describe --abbrev --dirty --always --tags)"
else
	VER="UNKNOWN"
endif

CFLAGS += -DVERSION=\"$(VER)\"

SRCS = attest.c command.c message.c

DEPDIR := .deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

COMPILE.c = $(CC) $(DEPFLAGS) $(CFLAGS) -c

%.o : %.c
%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(CFLAGS) $(OUTPUT_OPTION) $<

all: attest

attest: attest.o command.o message.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf *~ *.o .deps attest

$(DEPDIR): ; @mkdir -p $@

DEPFILES := $(SRCS:%.c=$(DEPDIR)/%.d)
$(DEPFILES):

include $(wildcard $(DEPFILES))
