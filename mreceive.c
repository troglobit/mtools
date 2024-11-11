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
                [-p PORT] [-s ADDR]\n\
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
  -s ADDRESS   Source IP address for source-specific filtering (SSM)\n\
  -v           Print version information.\n\n");

	return rc;
}

int main(int argc, char *argv[])
{
	inet_addr_t *source = NULL, group;
	inet_addr_t ifaddr[MAXIP];
	size_t num_ifaddr = 0;
	char msg[BUFSIZE];
	int starttime;
	int prev = 0;
	int ret, c;
	int sd;

	while ((c = getopt(argc, argv, "46g:hi:I:np:qs:v")) != EOF) {
		switch (c) {
		case '4':
			opt_family = AF_INET; /* for completeness */
			break;
		case '6':
			opt_family = AF_INET6;
			break;
		case 'g':
			group_addr = optarg;
			break;
		case 'h':
			return usage(0);
		case 'i':
			if (num_ifaddr >= NELEMS(ifaddr)) {
				fprintf(stderr, "Too many addresses, max %zu supported.\n", NELEMS(ifaddr));
				exit(1);
			}

			ret = inet_parse(&ifaddr[num_ifaddr], optarg, 0);
			if (ret)
				exit(1);

			opt_family = ifaddr[num_ifaddr++].ss_family;
			break;
		case 'I':
			if (opt_ifname) {
				fprintf(stderr, "Single interface expected\n");
				exit(1);
			}

			opt_ifname = optarg;
			break;
		case 'n':
			opt_isnum = 1;
			break;
		case 'p':
			group_port = atoi(optarg);
			break;
		case 'q':
			opt_verbose = 0;
			break;
		case 's':
			if (source) {
				fprintf(stderr, "Only single source filtering supported currently.\n");
				exit(1);
			}
			source = calloc(1, sizeof(inet_addr_t));
			if (!source) {
				perror("calloc");
				exit(1);
			}
			ret = inet_parse(source, optarg, 0);
			if (ret)
				exit(1);
			break;
		case 'v':
			printf("mreceive version %s\n", VERSION);
			return 0;
		default:
			fprintf(stderr, "wrong parameters!\n\n");
			return usage(1);
		}
	}

	if (group_addr == NULL) {
		if (opt_family == AF_INET)
			group_addr = TEST_ADDR_IPV4;
		else if (opt_family == AF_INET6)
			group_addr = TEST_ADDR_IPV6;
		else
			exit(1);
	}

	ret = inet_parse(&group, group_addr, group_port);
	if (ret)
		exit(1);

	if (group.ss_family == AF_INET6 && num_ifaddr) {
		fprintf(stderr, "Joining IPv6 groups by source address not supported, use -I\n");
		exit(1);
	}

	if (group.ss_family == AF_INET6 && !opt_ifname) {
		fprintf(stderr, "-I is mandatory with IPv6\n");
		exit(1);
	}

	/* get a datagram socket */
	sd = sock_create(&group, NULL);
	if (sd < 0)
		exit(1);

	/* join the multicast group. */
	ret = sock_mc_join(sd, source, &group, opt_ifname, num_ifaddr, ifaddr);
	if (ret)
		exit(1);

	logit("Now receiving from multicast group: [%s]:%d\n", group_addr, group_port);

	for (int i = 0;; i++) {
		char from_buf[INET_ADDRSTR_LEN];
		inet_addr_t from = group;
		static int counter = 1;
		const char *from_str;
		socklen_t len;

		/* receive from the multicast address */
		len = inet_addrlen(&from);
		ret = recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr *)&from, &len);
		if (ret < 0) {
			perror("recvfrom");
			exit(1);
		}

		from_str = inet_address(&from, from_buf, sizeof(from_buf));
		if (!from_str) {
			perror("inet_ntop");
			exit(1);
		}

		if (opt_isnum) {
			int now, curr = atoi(msg);
			struct timeval tv;

			gettimeofday(&tv, NULL);
			if (i == 0)
				/* 500 to adjust for already executed instructions */
				starttime = tv.tv_sec * 1000000 + tv.tv_usec - 500;

			now = tv.tv_sec * 1000000 + tv.tv_usec - starttime;
			logit("%5d\t[%s]:%5d\t%d.%03d\t%5u\n", counter, from_str, inet_port(&from),
			      now / 1000000, (now % 1000000) / 1000, curr);

			if (curr > prev + 1) {
				if (prev + 1 == curr - 1)
					printf("****************\nMessage not received: %d\n****************\n", prev + 1);
				else
					printf("****************\nMessages not received: %d to %d\n****************\n",
					       prev + 1, curr - 1);
			}
			if (curr == prev)
				printf("Duplicate message received: %d\n", curr);
			if (curr < prev)
				printf("****************\nGap detected: %d from %d\n****************\n", curr, prev);
			prev = curr;
		} else {
			logit("Receive msg %d from [%s]:%d: %s\n", counter, from_str, inet_port(&from), msg);
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
