/*
 * common.c -- Common functions and variables for mreceive.c and msend.c
 */

#include "common.h"

char *group_addr  = NULL;
int   group_port  = 4444;

int   opt_count   = 0;
int   opt_family  = AF_INET;
char *opt_ifaddr  = NULL;
char *opt_ifname  = NULL;
int   opt_isnum   = 0;
int   opt_join    = 0;
int   opt_period  = 1000;	/* msec */
int   opt_ttl     = 1;		/* default for multicast */
int   opt_verbose = 1;

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
