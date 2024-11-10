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

#include <getopt.h>
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

typedef struct {
	struct sock *s;
	struct sock *to;
	char *buf;
	int len;
	int num_pkts;
} param_t;

char *test_addr = NULL;
int   test_port = 4444;
int   ttl       = 1;
int   period    = 1000;		/* msec */
int   isnumber  = 0;
int   join_flag = 0;		/* not join */

param_t param;


void timer_cb(int signo)
{
	static int counter = 1;
	int ret;

	(void)signo;

	if (isnumber) {
		param.buf = (char *)(&counter);
		param.len = sizeof(counter);
		logit("Sending msg %d, TTL %d, to %s:%d\n", counter, ttl, test_addr, test_port);
	} else {
		logit("Sending msg %d, TTL %d, to %s:%d: %s\n", counter, ttl, test_addr, test_port, param.buf);
	}

	ret = mc_send(param.s, param.to, param.buf, param.len);
	if (ret < 0) {
		perror("sendto");
		exit(1);
	}

	if (counter == param.num_pkts)
		exit(1);

	counter++;
}

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

int main(int argc, char *argv[])
{
	static struct option opts[] = {
		{ "join",       no_argument,       NULL, 'j' },
		{ "text",       required_argument, NULL, 'T' },
		{ NULL,         0,                 NULL, 0   }
	};
	struct ip_address *saddr = NULL, mc;
	struct sock s = {}, to = {};
	const char *if_name = NULL;
	char buf[BUFSIZE] = { 0 };
	struct itimerval times;
	struct sigaction act;
	int family = AF_INET;
	sigset_t sigset;
	int num_pkts = 0;
	int ret, c;

	while ((c = getopt_long_only(argc, argv, "46c:g:hi:I:jnp:P:qt:T:v", opts, NULL)) != EOF) {
		switch (c) {
		case '4':
			family = AF_INET; /* for completeness */
			break;
		case '6':
			family = AF_INET6;
			break;
		case 'c':
			num_pkts = atoi(optarg);
			break;
		case 'g':
			test_addr = optarg;
			break;
		case 'h':
			return usage(0);
		case 'i':
			if (saddr) {
				printf("Single source address allowed\n");
				exit(1);
			}

			saddr = calloc(1, sizeof(*saddr));
			if (!saddr) {
				printf("Low memory\n");
				exit(1);
			}

			ret = ip_address_parse(optarg, saddr);
			if (ret)
				exit(1);
			family = saddr->family;
			break;
		case 'I':
			if (if_name) {
				printf("Single interface expected\n");
				exit(1);
			}

			if_name = optarg;
			break;
		case 'j':
			join_flag++;
			break;
		case 'n':
			isnumber = 1;
			break;
		case 'p':
			test_port = atoi(optarg);
			break;
		case 'P':
			period = atoi(optarg);
			break;
		case 'q':
			verbose = 0;
			break;
		case 't':
			ttl = atoi(optarg);
			break;
		case 'T':
			strcpy(buf, optarg);
			break;
		case 'v':
			printf("msend version %s\n", VERSION);
			return 0;
		default:
			printf("wrong parameters!\n\n");
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

	ret = ip_address_parse(test_addr, &mc);
	if (ret)
		exit(1);

	if (join_flag && mc.family == AF_INET6 && !if_name) {
		printf("-I is mandatory when joining IPv6 group\n");
		exit(1);
	}

	/* get a datagram socket */
	ret = socket_create(&s, mc.family, test_port, saddr, if_name);
	if (ret)
		exit(1);

	/* join the multicast group. */
	if (join_flag == 1) {
		ret = mc_join(&s, &mc, if_name, 0, NULL);
		if (ret)
			exit(1);
	}

	/* set TTL to traverse up to multiple routers */
	ret = mc_set_hop_limit(&s, ttl);
	if (ret)
		exit(1);

	/* enable loopback */
	ret = socket_set_loopback(&s, 1);
	if (ret)
		exit(1);

	/* assign our destination address */
	if (mc.family == AF_INET) {
		to.udp4.sin_addr = mc.addr;
		to.udp4.sin_port = htons(test_port);
		to.udp4.sin_family = AF_INET;
		to.addr_size = sizeof(struct sockaddr_in);
	} else {
		to.udp6.sin6_addr = mc.addr6;
		to.udp6.sin6_port = htons(test_port);
		to.udp6.sin6_family = AF_INET6;
		to.addr_size = sizeof(struct sockaddr_in6);
	}

	printf("Now sending to multicast group: %s\n", test_addr);

	period *= 1000;	/* convert to microsecond */
	if (period > 0) {
		/* block SIGALRM */
		sigemptyset(&sigset);
		sigaddset(&sigset, SIGALRM);
		sigprocmask(SIG_BLOCK, &sigset, NULL);

		/* set up handler for SIGALRM */
		act.sa_handler = &timer_cb;
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_SIGINFO;
		sigaction(SIGALRM, &act, NULL);
		/*
		 * set up interval timer
		 */
		times.it_value.tv_sec = 0;	/* wait a bit for system to "stabilize"  */
		times.it_value.tv_usec = 1;	/* tv_sec or tv_usec cannot be both zero */
		times.it_interval.tv_sec = (time_t)(period / 1000000);
		times.it_interval.tv_usec = (long)(period % 1000000);
		setitimer(ITIMER_REAL, &times, NULL);

		param.s = &s;
		param.to = &to;
		param.buf = buf;
		param.len = strlen(buf) + 1;
		param.num_pkts = num_pkts;

		/* now wait for the alarms */
		sigemptyset(&sigset);
		for (;;) {
			sigsuspend(&sigset);
		}
		return 0;
	} else {
		int i;

		for (i = 0; num_pkts && i < num_pkts; i++) {
			if (isnumber) {
				buf[3] = (unsigned char)(i >> 24);
				buf[2] = (unsigned char)(i >> 16);
				buf[1] = (unsigned char)(i >> 8);
				buf[0] = (unsigned char)(i);
				printf("Send out msg %d to %s:%d\n", i, test_addr, test_port);
			} else {
				printf("Send out msg %d to %s:%d: %s\n", i, test_addr, test_port, buf);
			}

			ret = mc_send(&s, &to, buf,
				      isnumber ? 4 : strlen(buf) + 1);
			if (ret < 0) {
				perror("sendto");
				exit(1);
			}
		}
	}

	return 0;
}

/**
 * Local Variables:
 *  version-control: t
 *  indent-tabs-mode: t
 *  c-file-style: "linux"
 * End:
 */
