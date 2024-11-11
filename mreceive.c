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

#include "common.h"

#define MAXIP     16


static int usage(int rc)
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

	return rc;
}

int main(int argc, char *argv[])
{
	unsigned char buf[BUFSIZE];
	const char *ifname = NULL;
	unsigned int numreceived;
	inet_addr_t ip[MAXIP];
	int rcvCountOld = 0;
	int rcvCountNew = 1;
	struct timeval tv;
	inet_addr_t group;
	int family = AF_INET;
	int starttime;
	int ipnum = 0;
	int ret, c;
	int i, sd;

	while ((c = getopt(argc, argv, "46g:hi:I:np:qv")) != EOF) {
		switch (c) {
		case '4':
			family = AF_INET; /* for completeness */
			break;
		case '6':
			family = AF_INET6;
			break;
		case 'g':
			test_addr = optarg;
			break;
		case 'h':
			return usage(0);
		case 'i':
			ret = inet_parse(&ip[ipnum], optarg, 0);
			if (ret)
				exit(1);

			family = ip[ipnum++].ss_family;
			break;
		case 'I':
			if (ifname) {
				fprintf(stderr, "Single interface expected\n");
				exit(1);
			}

			ifname = optarg;
			break;
		case 'n':
			isnumber = 1;
			break;
		case 'p':
			test_port = atoi(optarg);
			break;
		case 'q':
			verbose = 0;
			break;
		case 'v':
			printf("mreceive version %s\n", VERSION);
			return 0;
		default:
			fprintf(stderr, "wrong parameters!\n\n");
			return usage(1);
		}
	}

	if (test_addr == NULL) {
		if (family == AF_INET)
			test_addr = TEST_ADDR_IPV4;
		else if (family == AF_INET6)
			test_addr = TEST_ADDR_IPV6;
		else
			exit(1);
	}

	ret = inet_parse(&group, test_addr, test_port);
	if (ret)
		exit(1);

	if (group.ss_family == AF_INET6 && ipnum) {
		printf("Joining IPv6 groups by source address not supported, use -I\n");
		exit(1);
	}

	if (group.ss_family == AF_INET6 && !ifname) {
		printf("-I is mandatory with IPv6\n");
		exit(1);
	}

	/* get a datagram socket */
	sd = sock_create(&group, NULL);
	if (sd < 0)
		exit(1);

	/* join the multicast group. */
	ret = sock_mc_join(sd, &group, ifname, ipnum, ip);
	if (ret)
		exit(1);

	logit("Now receiving from multicast group: %s\n", test_addr);

	for (i = 0;; i++) {
		char from_buf[INET_ADDRSTR_LEN];
		inet_addr_t from = group;
		static int counter = 1;
		const char *from_str;
		socklen_t len;

		/* receive from the multicast address */
		len = inet_addrlen(&from);
		ret = recvfrom(sd, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);
		if (ret < 0) {
			perror("recvfrom");
			exit(1);
		}

		from_str = inet_address(&from, from_buf, sizeof(from_buf));
		if (!from_str) {
			perror("inet_ntop");
			exit(1);
		}

		if (isnumber) {
			int curtime;

			gettimeofday(&tv, NULL);

			if (i == 0)
				starttime = tv.tv_sec * 1000000 + tv.tv_usec;
			curtime = tv.tv_sec * 1000000 + tv.tv_usec - starttime;
			numreceived =
			    (unsigned int)buf[0] + ((unsigned int)(buf[1]) << 8) + ((unsigned int)(buf[2]) << 16) +
			    ((unsigned int)(buf[3]) >> 24);
			logit("%5d\t%s:%5d\t%d.%03d\t%5u\n", counter,
			      from_buf, inet_port(&from),
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
			logit("Receive msg %d from %s:%d: %s\n", counter, from_buf, inet_port(&from), buf);
		}
		counter++;
	}

	return 0;
}

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
