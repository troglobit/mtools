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

#include "common.h"


static int usage(int rc)
{
	printf("\
Usage:  msend [-46hnv] [-c NUM] [-g GROUP] [-p PORT] [-join] [-i ADDRESS]\n\
	      [-I INTERFACE] [-P PERIOD] [-t TTL] [-text \"text\"]\n\
\n\
  -4 | -6      Select IPv4 or IPv6, use with -I, when -i is not used\n\
  -c NUM       Number of packets to send. Default: send indefinitely\n\
  -g GROUP     IP multicast group address to send to.\n\
               Default for IPv4: 224.1.1.1, IPv6: ff2e::1\n\
  -h           This help text.\n\
  -i ADDRESS   IP address of the interface to use to send the packets.\n \
               The default is to use the system default interface.\n\
  -I INTERFACE The interface on which to send. Can be specified as an\n\
               alternative to -i.\n\
  -join        Multicast sender will join the multicast group.\n\
               By default a sender never joins the group.\n\
  -n           Encode -text argument as a number instead of a string.\n\
  -p PORT      UDP port number used in the multicast packets.  Default: 4444\n\
  -P PERIOD    Interval in milliseconds between packets.  Default 1000 msec\n\
  -q           Quiet, don't print 'Sedning msg ...' for every packet\n\
  -t TTL       The TTL value (1-255) used in the packets.  You must set\n\
               this higher if you want to route the traffic, otherwise\n\
               the first router will drop the packets!  Default: 1\n\
  -text \"text\" Specify a string to use as payload in the packets, also\n\
               displayed by the mreceive command.  Default: empty\n\
  -v           Print version information.\n\n");

	return rc;
}

static int do_send(int sd, inet_addr_t *to, char *msg, size_t len, int isnum)
{
	static int counter = 1;
	int ret;

	if (isnum)
		snprintf(msg, len, "%d", counter);

	ret = sendto(sd, msg, len, 0, (struct sockaddr *)to, inet_addrlen(to));
	if (ret < 0) {
		perror("sendto");
		exit(1);
	}

	return counter++;
}

int main(int argc, char *argv[])
{
	static struct option opts[] = {
		{ "join",       no_argument,       NULL, 'j' },
		{ "text",       required_argument, NULL, 'T' },
		{ NULL,         0,                 NULL, 0   }
	};
	inet_addr_t ifaddr, group;
	char msg[BUFSIZE] = { 0 };
	int ret, c, sd;

	while ((c = getopt_long_only(argc, argv, "46c:g:hi:I:jnp:P:qt:T:v", opts, NULL)) != EOF) {
		switch (c) {
		case '4':
			opt_family = AF_INET; /* for completeness */
			break;
		case '6':
			opt_family = AF_INET6;
			break;
		case 'c':
			opt_count = atoi(optarg);
			break;
		case 'g':
			group_addr = optarg;
			break;
		case 'h':
			return usage(0);
		case 'i':
			if (opt_ifaddr) {
				fprintf(stderr, "Single source address allowed\n");
				exit(1);
			}
			opt_ifaddr = strdup(optarg);
			if (!opt_ifaddr) {
				perror("strdup");
				exit(1);
			}
			break;
		case 'I':
			if (opt_ifname) {
				fprintf(stderr, "Single interface expected\n");
				exit(1);
			}
			opt_ifname = optarg;
			break;
		case 'j':
			opt_join++;
			break;
		case 'n':
			opt_isnum = 1;
			break;
		case 'p':
			group_port = atoi(optarg);
			break;
		case 'P':
			opt_period = atoi(optarg);
			break;
		case 'q':
			opt_verbose = 0;
			break;
		case 't':
			opt_ttl = atoi(optarg);
			break;
		case 'T':
			strlcpy(msg, optarg, sizeof(msg));
			break;
		case 'v':
			printf("msend version %s\n", VERSION);
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
	if (ret) {
		fprintf(stderr, "Group address %s not in known format\n", group_addr);
		exit(1);
	}

	/* Always derived from group */
	opt_family = group.ss_family;

	if (!opt_ifaddr) {
		if (opt_family == AF_INET)
			opt_ifaddr = "0.0.0.0";
		else
			opt_ifaddr = "::";
	}

	ret = inet_parse(&ifaddr, opt_ifaddr, group_port);
	if (ret) {
		fprintf(stderr, "IP address %s not in known format\n", opt_ifaddr);
		exit(1);
	}

	if (opt_join && group.ss_family == AF_INET6 && !opt_ifname) {
		fprintf(stderr, "-I is mandatory when joining IPv6 group\n");
		exit(1);
	}

	/* get a datagram socket */
	sd = sock_create(&ifaddr, opt_ifname);
	if (sd < 0)
		exit(1);

	/* join the multicast group we are sending to (usually not necessary!) */
	if (opt_join) {
		ret = sock_mc_join(sd, NULL, &group, opt_ifname, 0, NULL);
		if (ret)
			exit(1);
	}

	/* set TTL to traverse up to multiple routers */
	ret = sock_mc_ttl(sd, opt_ttl);
	if (ret)
		exit(1);

	/* enable loopback */
	ret = sock_mc_loop(sd, 1);
	if (ret)
		exit(1);

	logit("Now sending to multicast group: [%s]:%d\n", group_addr, group_port);

	opt_period *= 1000;	/* convert to microsecond */
	if (opt_period > 0) {
		struct itimerval it;
		sigset_t set;

		/* block SIGALRM */
		sigemptyset(&set);
		sigaddset(&set, SIGALRM);
		sigaddset(&set, SIGINT);
		sigprocmask(SIG_BLOCK, &set, NULL);

		/*
		 * set up interval timer, sends SIGALRM every opt_period msec
		 */
		it.it_value.tv_sec     = 0;	/* wait a bit for system to "stabilize"  */
		it.it_value.tv_usec    = 1;	/* tv_sec or tv_usec cannot be both zero */
		it.it_interval.tv_sec  = (time_t)(opt_period / 1000000);
		it.it_interval.tv_usec =   (long)(opt_period % 1000000);
		setitimer(ITIMER_REAL, &it, NULL);

		for (;;) {
			int signo = sigwaitinfo(&set, NULL);

			if (signo == SIGALRM) {
				ret = do_send(sd, &group, msg, sizeof(msg), opt_isnum);
				logit("Sending msg %d, TTL %d, to [%s]:%d: %s\n", ret, opt_ttl, group_addr, group_port, msg);
			} else {
				logit("Git signal %d, exiting!\n", signo);
				break;
			}

			if (ret == opt_count)
				break;
		}

		return 0;
	} else {
		for (int i = 0; i < opt_count; i++) {
			do_send(sd, &group, msg, sizeof(msg), opt_isnum);
			logit("Send out msg %d to [%s]:%d: %s\n", i, group_addr, group_port, msg);
		}
	}

	return 0;
}

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
