# -*-Makefile-*- for mtools
# Originally from http://www.cs.virginia.edu/~mngroup/software/

# Figure out root of library, unless used as submodule
ROOTDIR    ?= $(shell pwd)
VERSION    ?= 2.3-rc1
NAME        = mtools
PKG         = $(NAME)-$(VERSION)
ARCHIVE     = $(PKG).tar.gz

CC         ?= $(CROSS)gcc
CPPFLAGS   += -DVERSION=\"$(VERSION)\"
CPPFLAGS   += -W -Wall

EXEC       := msend mreceive ttcp
OBJS       := $(EXEC:=.o)
DEPS       := $(EXEC:=.d)

%.o: %.c
	@printf "  CC      $(subst $(ROOTDIR)/,,$(shell pwd)/)$@\n"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

%: %.o
	@printf "  LINK    $(subst $(ROOTDIR)/,,$(shell pwd)/)$@\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map -o $@ $^ $(LDLIBS$(LDLIBS-$(@)))


all: $(EXEC)

msend:    msend.o
mreceive: mreceive.o
ttcp:     ttcp.o

clean:
	@rm -f $(EXEC) $(OBJS)

distclean: clean
	@rm -f *.o *.d *~ *.map

dist:
	@echo "Building .xz tarball of $(PKG) in parent dir..."
	@git archive --format=tar --prefix=$(PKG)/ v$(VERSION) | gz >../$(ARCHIVE)
	@(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)

# Include automatically generated rules
-include $(DEPS)
