-include $(TOPDIR)/Make.defs

DEPPATH = --dep-path .
VPATH = .
CSRCS =

AZURE_DIR = ${shell pwd}/../..

include Make.defs

COBJS = $(CSRCS:.c=$(OBJEXT))

OBJS = $(COBJS)

BIN = libazure$(LIBEXT)

all:	$(BIN)

$(COBJS): %$(OBJEXT): %.c
	$(call COMPILE, $<, $@)

$(BIN): $(OBJS)
	$(call ARCHIVE, $@, $(OBJS))

.depend: Makefile $(SRCS)
	$(Q) $(MKDEP) $(DEPPATH) "$(CC)" -- $(CFLAGS) -- $(SRCS) >Make.dep
	$(Q) touch $@

depend: .depend

clean:
	$(call DELFILE, $(BIN))
	$(call CLEAN, $(AZURE_DIR))

distclean: clean
	$(call DELFILE, Make.dep)
	$(call DELFILE, .depend)

-include Make.dep
