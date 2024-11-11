/*
 * common.h -- Common header for mreceive.c and msend.c
 */

#ifndef MTOOLS_COMMON_H_
#define MTOOLS_COMMON_H_

#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "inet.h"
#include "sock.h"

#define TEST_ADDR_IPV4 "224.1.1.1"
#define TEST_ADDR_IPV6 "ff2e::1"

#define LOOPMAX        20
#define BUFSIZE        1024

#ifndef strlcpy			/* older glibc systems */
#define strlcpy strncpy
#endif

/* From The Practice of Programming, by Kernighan and Pike */
#ifndef NELEMS
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))
#endif

#define logit(fmt, args...) if (opt_verbose) {   \
	printf(fmt, ##args); fflush(stdout); \
}

extern char *group_addr;
extern int   group_port;

extern int   opt_count;
extern int   opt_family;
extern char *opt_ifaddr;
extern char *opt_ifname;
extern int   opt_isnum;
extern int   opt_join;
extern int   opt_period;
extern int   opt_ttl;
extern int   opt_verbose;

#endif /* MTOOLS_COMMON_H_ */

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
