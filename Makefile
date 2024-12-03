# -*-Makefile-*- for mtools
# Originally from http://www.cs.virginia.edu/~mngroup/software/

VERSION    ?= 3.2
NAME        = mtools
PKG         = $(NAME)-$(VERSION)
ARCHIVE     = $(PKG).tar.gz

CC         ?= $(CROSS)gcc
CPPFLAGS   += -D_GNU_SOURCE -DVERSION=\"$(VERSION)\"
CFLAGS     += -W -Wall -Wextra -g

prefix     ?= /usr/local
datadir    ?= $(prefix)/share/doc/mtools
mandir      = $(prefix)/share/man/man8

# ttcp is currently not part of the distribution because its not tested
# yet.  Please test and let me know at GitHub so I can include it! :)
EXEC       := msend mreceive
SHARED     := common.o inet.o sock.o
OBJS       := msend.o mreceive.o $(SHARED)
DEPS       := $(OBJS:.o=.d)
MANS        = $(addsuffix .8,$(EXEC))
DISTFILES   = ChangeLog.md README.md LICENSE.md

all: $(EXEC)

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -MMD -MP -o $@ $<

msend: msend.o $(SHARED)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map -o $@ msend.o $(SHARED) $(LDLIBS)

mreceive: mreceive.o $(SHARED)
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map -o $@ mreceive.o $(SHARED) $(LDLIBS)

ttcp: ttcp.o
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,-Map,$@.map -o $@ ttcp.o $(LDLIBS)

install: $(EXEC)
	install -d $(DESTDIR)$(prefix)/sbin
	install -d $(DESTDIR)$(datadir)
	install -d $(DESTDIR)$(mandir)
	for file in $(EXEC); do \
		install -s -m 0755 $$file $(DESTDIR)$(prefix)/sbin/$$file; \
	done
	for file in $(DISTFILES); do \
		install -m 0644 $$file $(DESTDIR)$(datadir)/$$file; \
	done
	for file in $(MANS); do \
		install -m 0644 $$file $(DESTDIR)$(mandir)/$$file; \
	done

uninstall:
	-for file in $(EXEC); do \
		(RM) $(DESTDIR)$(prefix)/sbin/$$file; \
	done
	-$(RM) -r $(DESTDIR)$(datadir)
	-for file in $(DISTFILES); do \
		$(RM) $(DESTDIR)$(datadir)/$$file; \
	done
	-for file in $(MANS); do \
		$(RM) $(DESTDIR)$(mandir)/$$file; \
	done

clean:
	rm -f $(EXEC) $(OBJS)

distclean: clean
	rm -f *.o *.d *~ *.map msend mreceive ttcp

dist:
	@if [ -e ../$(ARCHIVE) ]; then \
		echo "Distribution ../$(ARCHIVE) already exists."; \
		exit 1; \
	fi
	@echo "Building .gz tarball of $(PKG) in parent dir..."
	git archive --format=tar --prefix=$(PKG)/ v$(VERSION) | gzip >../$(ARCHIVE)
	(cd ..; md5sum $(ARCHIVE) | tee $(ARCHIVE).md5)
	(cd ..; sha256sum $(ARCHIVE) | tee $(ARCHIVE).sha256)

doc:
	for file in $(EXEC); do \
		mandoc -mdoc -T markdown $$file.8 > $$file\(8\).md; \
	done

# Include automatically generated rules
-include $(DEPS)
