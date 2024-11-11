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
#include <sys/time.h>

#include "inet.h"
#include "sock.h"

#define TEST_ADDR_IPV4 "224.1.1.1"
#define TEST_ADDR_IPV6 "ff2e::1"

#define LOOPMAX        20
#define BUFSIZE        1024

#define logit(fmt, args...) if (verbose) printf(fmt, ##args)

extern char *test_addr;
extern int   test_port;
extern int   isnumber;
extern int   verbose;

#endif /* MTOOLS_COMMON_H_ */

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
