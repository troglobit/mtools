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
unsigned long IP[MAXIP];
int NUM = 0;

void printHelp(void)
{
	printf("mreceive version %s\n\
Usage: mreceive [-g GROUP] [-p PORT] [-i ADDRESS ] ... [-i ADDRESS] [-n]\n\
       mreceive [-v | -h]\n\
\n\
  -g GROUP     IP multicast group address to listen to.  Default: 224.1.1.1\n\
  -p PORT      UDP port number used in the multicast packets.  Default: 4444\n\
  -i ADDRESS   IP addresses of one or more interfaces to listen for the given\n\
               multicast group.  Default: the system default interface.\n\
  -n           Interpret the contents of the message as a number instead of\n\
               a string of characters.  Use this with `msend -n`\n\
  -v           Print version information.\n\
  -h           Print the command usage.\n\n", VERSION);
}

static void igmp_join_by_saddr(int s, in_addr_t multiaddr, in_addr_t interface)
{
	struct ip_mreq mreq;
	int ret;

	mreq.imr_multiaddr.s_addr = multiaddr;
	mreq.imr_interface.s_addr = interface;

	ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			 (char *)&mreq, sizeof(mreq));
	if (ret == SOCKET_ERROR) {
		printf("setsockopt() IP_ADD_MEMBERSHIP failed.\n");
		exit(1);
	}
}

static void igmp_join_by_if_name(int s, in_addr_t multicast,
				 const char *if_name)
{
	struct ip_mreqn mreq = {};
	int if_index;
	int ret;

	if_index = if_nametoindex(if_name);
	if (!if_index) {
		perror("if_nametoindex");
		exit(1);
	}

	mreq.imr_multiaddr.s_addr = multicast;
	mreq.imr_ifindex = if_index;

	ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret) {
		perror("setsockopt() IP_ADD_MEMBERSHIP");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	struct sockaddr_in stLocal, stFrom;
	unsigned char achIn[BUFSIZE];
	const char *if_name;
	int s, i;
	int iTmp, iRet;
	int ipnum = 0;
	int ii;
	unsigned int numreceived;
	int rcvCountOld = 0;
	int rcvCountNew = 1;
	int starttime;
	int curtime;
	struct timeval tv;

/*
  if( argc < 2 ) {
    printHelp(); 
    return 1;
  }
*/

	ii = 1;

	if ((argc == 2) && (strcmp(argv[ii], "-v") == 0)) {
		printf("mreceive version 2.2\n");
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
		} else if (strcmp(argv[ii], "-i") == 0) {
			ii++;
			if ((ii < argc) && !(strchr(argv[ii], '-'))) {
				IP[ipnum] = inet_addr(argv[ii]);
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
			printHelp();
			return 1;
		}
	}

	/* get a datagram socket */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == INVALID_SOCKET) {
		printf("socket() failed.\n");
		exit(1);
	}

	/* avoid EADDRINUSE error on bind() */
	iTmp = TRUE;
	iRet = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&iTmp, sizeof(iTmp));
	if (iRet == SOCKET_ERROR) {
		printf("setsockopt() SO_REUSEADDR failed.\n");
		exit(1);
	}

	/* name the socket */
	stLocal.sin_family = AF_INET;
	stLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	stLocal.sin_port = htons(TEST_PORT);
	iRet = bind(s, (struct sockaddr *)&stLocal, sizeof(stLocal));
	if (iRet == SOCKET_ERROR) {
		printf("bind() failed.\n");
		exit(1);
	}

	/* join the multicast group. */
	if (if_name) {
		igmp_join_by_if_name(s, inet_addr(TEST_ADDR), if_name);
	} else {
		if (!ipnum) {		/* single interface */
			igmp_join_by_saddr(s, inet_addr(TEST_ADDR), INADDR_ANY);
		} else {
			for (i = 0; i < ipnum; i++) {
				igmp_join_by_saddr(s, inet_addr(TEST_ADDR),
						   IP[i]);
			}
		}
	}

	/* set TTL to traverse up to multiple routers */
	iTmp = TTL_VALUE;
	iRet = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&iTmp, sizeof(iTmp));
	if (iRet == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_TTL failed.\n");
		exit(1);
	}

	/* disable loopback */
	/* iTmp = TRUE; */
	iTmp = FALSE;
	iRet = setsockopt(s, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&iTmp, sizeof(iTmp));
	if (iRet == SOCKET_ERROR) {
		printf("setsockopt() IP_MULTICAST_LOOP failed.\n");
		exit(1);
	}

	printf("Now receiving from multicast group: %s\n", TEST_ADDR);

	for (i = 0;; i++) {
		socklen_t addr_size = sizeof(struct sockaddr_in);
		static int iCounter = 1;

		/* receive from the multicast address */

		iRet = recvfrom(s, achIn, BUFSIZE, 0, (struct sockaddr *)&stFrom, &addr_size);
		if (iRet < 0) {
			printf("recvfrom() failed.\n");
			exit(1);
		}

		if (NUM) {
			gettimeofday(&tv, NULL);

			if (i == 0)
				starttime = tv.tv_sec * 1000000 + tv.tv_usec;
			curtime = tv.tv_sec * 1000000 + tv.tv_usec - starttime;
			numreceived =
			    (unsigned int)achIn[0] + ((unsigned int)(achIn[1]) << 8) + ((unsigned int)(achIn[2]) << 16) +
			    ((unsigned int)(achIn[3]) >> 24);
			fprintf(stdout, "%5d\t%s:%5d\t%d.%03d\t%5d\n", iCounter, inet_ntoa(stFrom.sin_addr), ntohs(stFrom.sin_port),
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
			printf("Receive msg %d from %s:%d: %s\n",
			       iCounter, inet_ntoa(stFrom.sin_addr), ntohs(stFrom.sin_port), achIn);
		}
		iCounter++;
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
