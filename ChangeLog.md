Change Log
==========

All relevant changes are documented in this file.


[v3.2][] - 2024-12-03
---------------------

### Fixes

- Fix #8: ordering bug cause failure to detect address family in a basic
  `msend -g ff0e::181 -I eth0`.  Regression introduced in v3.1


[v3.1][] - 2024-11-11
---------------------

### Changes
- Add optional `-s ADDR` argument for SSM to `mreceive`, issue #3
- Add support for quiet (`-q`) mode operation
- Add credit to original authors in README and man pages
- Refactor the option parsers to use `getopt(3)`
- Major refactoring and coding style cleanup, splitting the code base up
  in more files, renaming internal APIs, all to be able to use RFC3678
  style APIs for source filtering, issue #3

### Fixes
- Boundary checking, max number of interfaces, internal buffer lengths
- Fixes for building on non-Linux systems, tested on FreeBSD 11.0


[v3.0][] - 2024-02-24
---------------------

This release adds IPv6 support, contributed by Vladimir Oltean.

### Changes

- Add `-I INTERFACE` option to bind to a specific interface for send/receive
- Add `-c NUM` option to only send a limited number of packets
- Add `-4` and `-6` options to select default group to send/receive with `I`
- Sort command line options alphabetically
- Update documentation

### Fixes

- Fix `-v` to print correct version instead of hard-coded old version
- Bind to port after binding to device to receive only test packets


[v2.3][] - 2015-07-30
---------------------

First release of the mtools multicast testing suite (`msend` and `mreceive`)
after importing the sources from the [Multimedia Networks Group, University of
Virginia](http://www.cs.virginia.edu/~mngroup/software/).

> **Note:** First effort to adapt `ttcp.c` as well, but not tested yet,
> so not part of the build/install system.

### Changes

- Code cleanup and modernization to be able to build with a modern
  (2015) GCC and Clang
- Reindent code to use KNF
- Simplifiy and clean up online, -h, command usage help
- Removed old support for Windows.  May add Cygwin support in the future
- Cleanup of documentation, add common README and create real man pages:
  `msend.8` and `mreceive.8`
- Create real public domain LICENSE file
- Tested on Ubuntu Linux 14.04 (Amd64) and FreeBSD (Amd64)


[v3.2]: https://github.com/troglobit/mtools/compare/v3.1...v3.2
[v3.1]: https://github.com/troglobit/mtools/compare/v3.0...v3.1
[v3.0]: https://github.com/troglobit/mtools/compare/v2.3...v3.0
[v2.3]: https://github.com/troglobit/mtools/compare/v2.2...v2.3
[v2.2]: https://github.com/troglobit/mtools/compare/v2.1...v2.2
[v2.1]: https://github.com/troglobit/mtools/compare/v2.0...v2.1
