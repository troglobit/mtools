# -*-Makefile-*- for mtools
# Originally from http://www.cs.virginia.edu/~mngroup/software/

# Figure out root of library, unless used as submodule
ROOTDIR    ?= $(shell pwd)
VERSION    ?= 3.0-beta1
NAME        = mtools
PKG         = $(NAME)-$(VERSION)
ARCHIVE     = $(PKG).tar.gz

CC         ?= $(CROSS)gcc
CPPFLAGS   += -DVERSION=\"$(VERSION)\"
CFLAGS     += -W -Wall -Wextra -g

prefix     ?= /usr/local
sysconfdir ?= /etc
datadir     = $(prefix)/share/doc/mtools
mandir      = $(prefix)/share/man/man8

# ttcp is currently not part of the distribution because its not tested
# yet.  Please test and let me know at GitHub so I can include it! :)
EXEC       := msend mreceive
OBJS       := msend.o mreceive.o common.o
DEPS       := msend.d mreceive.d common.d
MANS        = $(addsuffix .8,$(EXEC))
DISTFILES   = ChangeLog.md README.md LICENSE.md

all: $(EXEC)

.c.o:
	@printf "  CC      $@\n"
	@$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

.o:
	@printf "  LINK    $@\n"
	@$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map -o $@ $^ $(LDLIBS$(LDLIBS-$(@)))

msend:    msend.o common.o
mreceive: mreceive.o common.o
ttcp:     ttcp.o

install: $(EXEC)
	@printf "  INSTALL $(DESTDIR)$(prefix) ...\n"
	@install -d $(DESTDIR)$(prefix)/sbin
	@install -d $(DESTDIR)$(datadir)
	@install -d $(DESTDIR)$(mandir)
	@for file in $(EXEC); do \
		install -s -m 0755 $$file $(DESTDIR)$(prefix)/sbin/$$file; \
	done
	@for file in $(DISTFILES); do \
		install -m 0644 $$file $(DESTDIR)$(datadir)/$$file; \
	done
	@for file in $(MANS); do \
		install -m 0644 $$file $(DESTDIR)$(mandir)/$$file; \
	done

uninstall:
	@printf "  UNINST  $(DESTDIR)$(prefix) ...\n"
	-@for file in $(EXEC); do \
		(RM) $(DESTDIR)$(prefix)/sbin/$$file; \
	done
	-@$(RM) -r $(DESTDIR)$(datadir)
	@for file in $(DISTFILES); do \
		$(RM) $(DESTDIR)$(datadir)/$$file; \
	done
	-@for file in $(MANS); do \
		$(RM) $(DESTDIR)$(mandir)/$$file; \
	done

clean:
	@rm -f $(EXEC) $(OBJS)

distclean: clean
	@rm -f *.o *.d *~ *.map msend mreceive ttcp

dist:
	@if [ -e ../$(ARCHIVE) ]; then \
		echo "Distribution ../$(ARCHIVE) already exists."; \
		exit 1; \
	fi
	@echo "Building .gz tarball of $(PKG) in parent dir..."
	@git archive --format=tar --prefix=$(PKG)/ v$(VERSION) | gzip >../$(ARCHIVE)
	@(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)

# Include automatically generated rules
-include $(DEPS)
