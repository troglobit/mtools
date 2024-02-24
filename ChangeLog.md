Change Log
==========

All relevant changes are documented in this file.


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

[v2.3]: https://github.com/troglobit/mtools/compare/v2.2...v2.3
[v2.2]: https://github.com/troglobit/mtools/compare/v2.1...v2.2
[v2.1]: https://github.com/troglobit/mtools/compare/v2.0...v2.1
