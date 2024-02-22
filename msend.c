/*
 * msend.c  -- Sends UDP packets to a multicast group
 * 
 * (c)  Jianping Wang, Yvan Pointurier, Jorg Liebeherr, 2002
 *      Multimedia Networks Group, University of Virginia
 *
 * SOURCE CODE RELEASED TO THE PUBLIC DOMAIN
 * 
 * version 2.0 - 5/20/2002 
 * version 2.1 - 12/4/2002  
 * 	By default, msend does not join multicast group. If  -join option is 
 * 	given, msend joins the multicast group. 
 * version 2.2 - 05/17/2003  
 *      Most commandline parameters are assigned default values. The 
 *      usage information is changed according to README_msend.txt
 * 
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
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
#define LOOPMAX   20
#define BUFSIZE   1024

#define TEST_ADDR_IPV4 "224.1.1.1"
#define TEST_ADDR_IPV6 "FF02::1:1"

char *TEST_ADDR = NULL;
int TEST_PORT = 4444;
int TTL_VALUE = 1;
int SLEEP_TIME = 1000;
int NUM = 0;

int join_flag = 0;		/* not join */

typedef struct timerhandler_s {
	struct sock *s;
	struct sock *to;
	char *achOut;
	int len;
	int num_pkts;
} timerhandler_t;
timerhandler_t handler_par;
void timerhandler();

void printHelp(void)
{
	printf("msend version %s\n\
Usage:  msend [-g GROUP] [-p PORT] [-join] [-i ADDRESS] [-t TTL] [-P PERIOD]\n\
	      [-text \"text\"|-n]\n\
	msend [-v | -h]\n\
\n\
  -g GROUP     IP multicast group address to send to.\n\
               Default: IPv4: 224.1.1.1, IPv6: FF02::1:1\n\
  -p PORT      UDP port number used in the multicast packets.  Default: 4444\n\
  -i ADDRESS   IP address of the interface to use to send the packets.\n\
               The default is to use the system default interface.\n\
  -I interface The interface on which to send. Can be specified as an\n\
               alternative to -i.\n\
  -join        Multicast sender will join the multicast group.\n\
               By default a sender never joins the group.\n\
  -P PERIOD    Interval in milliseconds between packets.  Default 1000 msec\n\
  -t TTL       The TTL value (1-255) used in the packets.  You must set\n\
               this higher if you want to route the traffic, otherwise\n\
               the first router will drop the packets!  Default: 1\n\
  -text \"text\" Specify a string to use as payload in the packets, also\n\
               displayed by the mreceive command.  Default: empty\n\
  -c           Number of packets to send. Default: send indefinitely\n\
  -n           Encode -text argument as a number instead of a string.\n\
  -v           Print version information.\n\
  -h           Print the command usage.\n\n", VERSION);
}

int main(int argc, char *argv[])
{
	struct ip_address *saddr = NULL, mc;
	struct sock s = {}, to = {};
	const char *if_name = NULL;
	char achOut[BUFSIZE] = "";
	int ii = 1;
	struct itimerval times;
	sigset_t sigset;
	struct sigaction act;
	int num_pkts = 0;
	int ret, i;

	if ((argc == 2) && (strcmp(argv[ii], "-v") == 0)) {
		printf("msend version 2.2\n");
		return 0;
	}
	if ((argc == 2) && (strcmp(argv[ii], "-h") == 0)) {
		printHelp();
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
		} else if (strcmp(argv[ii], "-join") == 0) {
			join_flag++;;
			ii++;
		} else if (strcmp(argv[ii], "-i") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				if (saddr) {
					printf("Single source address allowed\n");
					exit(1);
				}

				saddr = calloc(1, sizeof(*saddr));
				if (!saddr) {
					printf("Low memory\n");
					exit(1);
				}

				ret = ip_address_parse(argv[ii], saddr);
				if (ret)
					exit(1);

				ii++;
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
		} else if (strcmp(argv[ii], "-t") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				TTL_VALUE = atoi(argv[ii]);
				ii++;
			}
		} else if (strcmp(argv[ii], "-P") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				SLEEP_TIME = atoi(argv[ii]);
				ii++;
			}
		} else if (strcmp(argv[ii], "-n") == 0) {
			ii++;
			NUM = 1;
			ii++;
		} else if (strcmp(argv[ii], "-c") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				num_pkts = atoi(argv[ii]);
				ii++;
			}
		} else if (strcmp(argv[ii], "-text") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				strcpy(achOut, argv[ii]);
				ii++;
			}
		} else {
			printf("wrong parameters!\n\n");
			printHelp();
			return 1;
		}
	}

	if(TEST_ADDR == NULL) {
		if(saddr->family == AF_INET)
			TEST_ADDR = TEST_ADDR_IPV4;
		else if(saddr->family == AF_INET6)
			TEST_ADDR = TEST_ADDR_IPV6;
		else
			exit(1);
	}

	ret = ip_address_parse(TEST_ADDR, &mc);
	if (ret)
		exit(1);

	if (join_flag && mc.family == AF_INET6 && !if_name) {
		printf("-I is mandatory when joining IPv6 group\n");
		exit(1);
	}

	/* get a datagram socket */
	ret = socket_create(&s, mc.family, TEST_PORT, saddr, if_name);
	if (ret)
		exit(1);

	/* join the multicast group. */
	if (join_flag == 1) {
		ret = mc_join(&s, &mc, if_name, 0, NULL);
		if (ret)
			exit(1);
	}

	/* set TTL to traverse up to multiple routers */
	ret = mc_set_hop_limit(&s, TTL_VALUE);
	if (ret)
		exit(1);

	/* enable loopback */
	ret = socket_set_loopback(&s, 1);
	if (ret)
		exit(1);

	/* assign our destination address */
	if (mc.family == AF_INET) {
		to.udp4.sin_addr = mc.addr;
		to.udp4.sin_port = htons(TEST_PORT);
		to.udp4.sin_family = AF_INET;
		to.addr_size = sizeof(struct sockaddr_in);
	} else {
		to.udp6.sin6_addr = mc.addr6;
		to.udp6.sin6_port = htons(TEST_PORT);
		to.udp6.sin6_family = AF_INET6;
		to.addr_size = sizeof(struct sockaddr_in6);
	}

	printf("Now sending to multicast group: %s\n", TEST_ADDR);

	SLEEP_TIME *= 1000;	/* convert to microsecond */
	if (SLEEP_TIME > 0) {
		/* block SIGALRM */
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGALRM);
		sigprocmask(SIG_BLOCK, &sigset, NULL);

		/* set up handler for SIGALRM */
		act.sa_handler = &timerhandler;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGALRM, &act, NULL);
		/*
		 * set up interval timer
		 */
		times.it_value.tv_sec = 0;	/* wait a bit for system to "stabilize"  */
		times.it_value.tv_usec = 1;	/* tv_sec or tv_usec cannot be both zero */
		times.it_interval.tv_sec = (time_t)(SLEEP_TIME / 1000000);
		times.it_interval.tv_usec = (long)(SLEEP_TIME % 1000000);
		setitimer(ITIMER_REAL, &times, NULL);

		handler_par.s = &s;
		handler_par.to = &to;
		handler_par.achOut = achOut;
		handler_par.len = strlen(achOut) + 1;
		handler_par.num_pkts = num_pkts;

		/* now wait for the alarms */
		sigemptyset(&sigset);
		for (;;) {
			sigsuspend(&sigset);
		}
		return 0;
	} else {
		for (i = 0; num_pkts && i < num_pkts; i++) {
			if (NUM) {
				achOut[3] = (unsigned char)(i >> 24);
				achOut[2] = (unsigned char)(i >> 16);
				achOut[1] = (unsigned char)(i >> 8);
				achOut[0] = (unsigned char)(i);
				printf("Send out msg %d to %s:%d\n", i, TEST_ADDR, TEST_PORT);
			} else {
				printf("Send out msg %d to %s:%d: %s\n", i, TEST_ADDR, TEST_PORT, achOut);
			}

			ret = mc_send(&s, &to, achOut,
				      NUM ? 4 : strlen(achOut) + 1);
			if (ret < 0) {
				perror("sendto");
				exit(1);
			}
		}		/* end for(;;) */
	}

	return 0;
}				/* end main() */

void timerhandler(void)
{
	static int iCounter = 1;
	int ret;

	if (NUM) {
		handler_par.achOut = (char *)(&iCounter);
		handler_par.len = sizeof(iCounter);
		printf("Sending msg %d, TTL %d, to %s:%d\n", iCounter, TTL_VALUE, TEST_ADDR, TEST_PORT);
	} else {
		printf("Sending msg %d, TTL %d, to %s:%d: %s\n", iCounter, TTL_VALUE, TEST_ADDR, TEST_PORT, handler_par.achOut);
	}

	ret = mc_send(handler_par.s, handler_par.to, handler_par.achOut,
		      handler_par.len);
	if (ret < 0) {
		perror("sendto");
		exit(1);
	}

	if (iCounter == handler_par.num_pkts)
		exit(1);

	iCounter++;
	return;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
