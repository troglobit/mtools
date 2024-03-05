/*
 * mreceive.c  -- Prints UDP messages received from a multicast group. 
 * 
 * (c)  Jianping Wang, Yvan Pointurier, Jorg Liebeherr, 2002
 *      Multimedia Networks Group, University of Virginia
 *
 * SOURCE CODE RELEASED TO THE PUBLIC DOMAIN
 * 
 * version 2.0 - 5/20/2002
 * version 2.1 - 12/4/2002
 *	Update version display. 
 * version 2.2 - 05/17/2003
 *      Assign default values to parameters . The usage information is 
 *      changed according to README_mreceive.txt
 * 
 * Based on this public domain program:
 * u_mctest.c            (c) Bob Quinn           2/4/97
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "common.h"

#define TRUE 1
#define FALSE 0
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#define BUFSIZE   1024
#define TTL_VALUE 2
#define LOOPMAX   20
#define MAXIP     16
#define TEST_ADDR_IPV4 "224.1.1.1"
#define TEST_ADDR_IPV6 "ff2e::1"

char *TEST_ADDR = NULL;
int TEST_PORT = 4444;
struct ip_address IP[MAXIP];
int NUM = 0;

void usage(void)
{
	printf("\
Usage: mreceive [-46hnv] [-g GROUP] [-i ADDR] ... [-i ADDR] [-I INTERFACE]\n\
                [-p PORT]\n\
\n\
  -4 | -6      Select IPv4 or IPv6, use with -I, when -i is not used\n\
  -g GROUP     IP multicast group address to listen to.\n\
               Default for IPv4: 224.1.1.1, IPv6: ff2e::1\n\
  -h           This help text.\n\
  -i ADDRESS   IP addresses of one or more interfaces to listen for the given\n\
               multicast group.  Default: the system default interface.\n\
  -I INTERFACE The interface on which to receive. Can be specified as an\n\
               alternative to -i.\n\
  -n           Interpret the contents of the message as a number instead of\n\
               a string of characters.  Use this with `msend -n`\n\
  -p PORT      UDP port number used in the multicast packets.  Default: 4444\n\
  -q           Quiet, don't print every received packet, errors still printed\n\
  -v           Print version information.\n\n");
}

int main(int argc, char *argv[])
{
	unsigned char buf[BUFSIZE];
	const char *if_name = NULL;
	unsigned int numreceived;
	struct ip_address mc;
	int family = AF_INET;
	struct sock s, from;
	int rcvCountOld = 0;
	int rcvCountNew = 1;
	struct timeval tv;
	int starttime;
	int ipnum = 0;
	int opt = 1;
	int ret;
	int i;

	if ((argc == 2) && (strcmp(argv[opt], "-v") == 0)) {
		printf("mreceive version %s\n", VERSION);
		return 0;
	}
	if ((argc == 2) && (strcmp(argv[opt], "-h") == 0)) {
		usage();
		return 0;
	}


	while (opt < argc) {
		if (strcmp(argv[opt], "-4") == 0) {
			family = AF_INET; /* for completeness */
			opt++;
		} else if (strcmp(argv[opt], "-6") == 0) {
			family = AF_INET6;
			opt++;
		} else if (strcmp(argv[opt], "-g") == 0) {
			opt++;
			if ((opt < argc) && !(strchr(argv[opt], '-'))) {
				TEST_ADDR = argv[opt];
				opt++;
			}
		} else if (strcmp(argv[opt], "-p") == 0) {
			opt++;
			if ((opt < argc) && !(strchr(argv[opt], '-'))) {
				TEST_PORT = atoi(argv[opt]);
				opt++;
			}
		} else if (strcmp(argv[opt], "-q") == 0) {
			opt++;
			verbose = 0;
		} else if (strcmp(argv[opt], "-i") == 0) {
			opt++;
			if ((opt < argc) && !(strchr(argv[opt], '-'))) {
				ret = ip_address_parse(argv[opt], &IP[ipnum]);
				if (ret)
					exit(1);

				family = IP[ipnum].family;
				opt++;
				ipnum++;
			}
		} else if (strcmp(argv[opt], "-I") == 0) {
			opt++;
			if (opt < argc) {
				if (if_name) {
					printf("Single interface expected\n");
					exit(1);
				}

				if_name = argv[opt];
				opt++;
			}
		} else if (strcmp(argv[opt], "-n") == 0) {
			opt++;
			NUM = 1;
		} else {
			printf("wrong parameters!\n\n");
			usage();
			return 1;
		}
	}

	if (TEST_ADDR == NULL) {
		if (family == AF_INET)
			TEST_ADDR = TEST_ADDR_IPV4;
		else if (family == AF_INET6)
			TEST_ADDR = TEST_ADDR_IPV6;
		else
			exit(1);
	}

	ret = ip_address_parse(TEST_ADDR, &mc);
	if (ret)
		exit(1);

	if (mc.family == AF_INET6 && ipnum) {
		printf("Joining IPv6 groups by source address not supported, use -I\n");
		exit(1);
	}

	if (mc.family == AF_INET6 && !if_name) {
		printf("-I is mandatory with IPv6\n");
		exit(1);
	}

	/* get a datagram socket */
	ret = socket_create(&s, mc.family, TEST_PORT, NULL, NULL);
	if (ret)
		exit(1);

	/* join the multicast group. */
	ret = mc_join(&s, &mc, if_name, ipnum, IP);
	if (ret)
		exit(1);

	/* set TTL to traverse up to multiple routers */
	ret = mc_set_hop_limit(&s, TTL_VALUE);
	if (ret)
		exit(1);

	logit("Now receiving from multicast group: %s\n", TEST_ADDR);

	for (i = 0;; i++) {
		char from_buf[INET6_ADDRSTRLEN];
		static int counter = 1;
		const char *addr_str;

		/* receive from the multicast address */

		ret = mc_recv(&s, buf, BUFSIZE, &from);
		if (ret < 0) {
			perror("recvfrom");
			exit(1);
		}

		if (mc.family == AF_INET) {
			addr_str = inet_ntop(AF_INET, &from.udp4.sin_addr,
					     from_buf, INET6_ADDRSTRLEN);
		} else {
			addr_str = inet_ntop(AF_INET6, &from.udp6.sin6_addr,
					     from_buf, INET6_ADDRSTRLEN);
		}
		if (!addr_str) {
			perror("inet_ntop");
			exit(1);
		}

		if (NUM) {
			int curtime;

			gettimeofday(&tv, NULL);

			if (i == 0)
				starttime = tv.tv_sec * 1000000 + tv.tv_usec;
			curtime = tv.tv_sec * 1000000 + tv.tv_usec - starttime;
			numreceived =
			    (unsigned int)buf[0] + ((unsigned int)(buf[1]) << 8) + ((unsigned int)(buf[2]) << 16) +
			    ((unsigned int)(buf[3]) >> 24);
			logit("%5d\t%s:%5d\t%d.%03d\t%5u\n", counter,
			      from_buf, socket_get_port(&from),
			      curtime / 1000000, (curtime % 1000000) / 1000, numreceived);
			fflush(stdout);
			rcvCountNew = numreceived;
			if (rcvCountNew > rcvCountOld + 1) {
				if (rcvCountOld + 1 == rcvCountNew - 1)
					printf("****************\nMessage not received: %d\n****************\n", rcvCountOld + 1);
				else
					printf("****************\nMessages not received: %d to %d\n****************\n",
					       rcvCountOld + 1, rcvCountNew - 1);
			}
			if (rcvCountNew == rcvCountOld) {
				printf("Duplicate message received: %d\n", rcvCountNew);
			}
			if (rcvCountNew < rcvCountOld) {
				printf("****************\nGap detected: %d from %d\n****************\n", rcvCountNew, rcvCountOld);
			}
			rcvCountOld = rcvCountNew;
		} else {
			logit("Receive msg %d from %s:%d: %s\n", counter, from_buf, socket_get_port(&from), buf);
		}
		counter++;
	}

	return 0;
}				/* end main() */

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
