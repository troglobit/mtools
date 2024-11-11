/*
 * sock.c -- Helper functions for working with multicast sockets
 */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <net/if.h>

#include "sock.h"


int sock_create(inet_addr_t *ina, const char *ifname)
{
	int sd, on = 1;

	sd = socket(ina->ss_family, SOCK_DGRAM, 0);
	if (sd < 0) {
		perror("socket");
		return sd;
	}

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
		perror("setsockopt() SO_REUSEADDR");
		close(sd);
		return -1;
	}

	if (ifname) {
		/* Bind to device, required for IPv6 link-local addresses */
		if (setsockopt(sd, SOL_SOCKET, SO_BINDTODEVICE, ifname, IFNAMSIZ - 1)) {
			perror("setsockopt() SO_BINDTODEVICE");
			close(sd);
			return -1;
		}
	}

	if (bind(sd, (struct sockaddr *)ina, inet_addrlen(ina))) {
		perror("bind");
		close(sd);
		return -1;
	}

	return sd;
}

int sock_family(int sd)
{
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);

	if (getsockname(sd, (struct sockaddr *)&ss, &len) == -1) {
		perror("getsockname");
		return -1;
	}

	return ss.ss_family;
}

int sock_mc_loop(int sd, int loop)
{
	int ret = -1;

	switch (sock_family(sd)) {
	case AF_INET:
		ret = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));
		if (ret)
			perror("setsockopt IP_MULTICAST_LOOP");
		break;
	case AF_INET6:
		ret = setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop, sizeof(loop));
		if (ret)
			perror("setsockopt IPV6_MULTICAST_LOOP");
		break;
	default:
		errno = EAFNOSUPPORT;
		perror("getsockname");
	}

	return ret;
}

int sock_mc_ttl(int sd, int ttl)
{
	int ret = -1;

	switch (sock_family(sd)) {
	case AF_INET:
		ret = setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
		if (ret)
			perror("setsockopt() IP_MULTICAST_TTL");
		break;
	case AF_INET6:
		ret = setsockopt(sd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &ttl, sizeof(ttl));
		if (ret)
			perror("setsockopt() IPV6_MULTICAST_HOPS");
		break;
	default:
		errno = EAFNOSUPPORT;
		perror("getsockname");
	}

	return ret;
}

/* Classic IPv4 ASM (*,G) join for compat only, see below for new RFC3569 API */
static int igmp_join(int sd, const inet_addr_t *group, const struct in_addr *ina)
{
	const struct sockaddr_in *sin = (struct sockaddr_in *)group;
	struct ip_mreq mreq = {
		.imr_multiaddr = sin->sin_addr,
		.imr_interface = *ina,
	};
	int ret, off = 0;

	ret = setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret) {
		perror("setsockopt() IP_ADD_MEMBERSHIP");
		return -1;
	}

	return sock_mc_loop(sd, off);
}

static int mc_join_compat(int sd, const inet_addr_t *group, int num, inet_addr_t *addrs)
{
	int i, ret = 0;

	if (!num) {
		struct in_addr s = {
			.s_addr = INADDR_ANY,
		};

		return igmp_join(sd, group, &s);
	}

	for (i = 0; i < num; i++) {
		struct sockaddr_in *sin = (struct sockaddr_in *)&addrs[i];

		ret = igmp_join(sd, group, &sin->sin_addr);
		if (ret)
			break;
	}

	return ret;
}

int sock_mc_join(int sd, const inet_addr_t *group, const char *ifname,
	    int num_saddrs, inet_addr_t *saddrs)
{
	struct group_req gr;
	int ifindex, proto;

	if (!ifname) {
		if (group->ss_family == AF_INET6) {
			fputs("Need an interface for joining IPv6 groups.\n", stderr);
			return -1;
		}

		return mc_join_compat(sd, group, num_saddrs, saddrs);
	}

	ifindex = if_nametoindex(ifname);
	if (!ifindex) {
		perror("if_nametoindex");
		return -1;
	}

	if (group->ss_family == AF_INET6)
		proto = IPPROTO_IPV6;
	else
		proto = IPPROTO_IP;

	gr.gr_interface    = ifindex;
	gr.gr_group        = *group;

	if (setsockopt(sd, proto, MCAST_JOIN_GROUP, &gr, sizeof(gr))) {
		perror("setsockopt MCAST_JOIN_GROUP");
		return -1;
	}

	return sock_mc_loop(sd, 0);
}

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
