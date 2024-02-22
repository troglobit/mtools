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

char *TEST_ADDR = "224.1.1.1";
int TEST_PORT = 4444;
struct ip_address IP[MAXIP];
int NUM = 0;

void usage(void)
{
	printf("\
Usage: mreceive [-hnv] [-g GROUP] [-i ADDR] ... [-i ADDR] [-I INTERFACE]\n\
                [-p PORT]\n\
\n\
  -g GROUP     IP multicast group address to listen to.  Default: 224.1.1.1\n\
  -h           This help text.\n\
  -i ADDRESS   IP addresses of one or more interfaces to listen for the given\n\
               multicast group.  Default: the system default interface.\n\
  -I INTERFACE The interface on which to receive. Can be specified as an\n\
               alternative to -i.\n\
  -n           Interpret the contents of the message as a number instead of\n\
               a string of characters.  Use this with `msend -n`\n\
  -p PORT      UDP port number used in the multicast packets.  Default: 4444\n\
  -v           Print version information.\n\n");
}

int main(int argc, char *argv[])
{
	unsigned char buf[BUFSIZE];
	const char *if_name = NULL;
	struct ip_address mc;
	struct sock s, from;
	int ipnum = 0;
	int ii;
	unsigned int numreceived;
	int rcvCountOld = 0;
	int rcvCountNew = 1;
	int starttime;
	int curtime;
	struct timeval tv;
	int ret;
	int i;

/*
  if( argc < 2 ) {
    usage(); 
    return 1;
  }
*/

	ii = 1;

	if ((argc == 2) && (strcmp(argv[ii], "-v") == 0)) {
		printf("mreceive version %s\n", VERSION);
		return 0;
	}
	if ((argc == 2) && (strcmp(argv[ii], "-h") == 0)) {
		usage();
		return 0;
	}


	while (ii < argc) {
		if (strcmp(argv[ii], "-g") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				TEST_ADDR = argv[ii];
				ii++;
			}
		} else if (strcmp(argv[ii], "-p") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				TEST_PORT = atoi(argv[ii]);
				ii++;
			}
		} else if (strcmp(argv[ii], "-i") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				ret = ip_address_parse(argv[ii], &IP[ipnum]);
				if (ret)
					exit(1);

				ii++;
				ipnum++;
			}
		} else if (strcmp(argv[ii], "-I") == 0) {
			ii++;
			if (ii < argc) {
				if (if_name) {
					printf("Single interface expected\n");
					exit(1);
				}

				if_name = argv[ii];
				ii++;
			}
		} else if (strcmp(argv[ii], "-n") == 0) {
			ii++;
			NUM = 1;
		} else {
			printf("wrong parameters!\n\n");
			usage();
			return 1;
		}
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

	printf("Now receiving from multicast group: %s\n", TEST_ADDR);

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
			gettimeofday(&tv, NULL);

			if (i == 0)
				starttime = tv.tv_sec * 1000000 + tv.tv_usec;
			curtime = tv.tv_sec * 1000000 + tv.tv_usec - starttime;
			numreceived =
			    (unsigned int)buf[0] + ((unsigned int)(buf[1]) << 8) + ((unsigned int)(buf[2]) << 16) +
			    ((unsigned int)(buf[3]) >> 24);
			fprintf(stdout, "%5d\t%s:%5d\t%d.%03d\t%5u\n", counter,
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
			printf("Receive msg %d from %s:%d: %s\n", counter, from_buf, socket_get_port(&from), buf);
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
