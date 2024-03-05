/*
 * common.c -- Common functions for mreceive.c and msend.c
 */
#include <arpa/inet.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

int verbose = 1;


int ip_address_parse(const char *string, struct ip_address *ip)
{
	int ret;

	ret = inet_pton(AF_INET6, string, &ip->addr6);
	if (ret > 0) {
		ip->family = AF_INET6;
	} else {
		ret = inet_pton(AF_INET, string, &ip->addr);
		if (ret > 0) {
			ip->family = AF_INET;
		} else {
			fprintf(stderr, "IP address %s not in known format\n",
			        string);
			return -1;
		}
	}

	return 0;
}

int socket_create(struct sock *s, int family, int port,
		  struct ip_address *saddr, const char *if_name)
{
	struct sockaddr *serv_addr;
	int sockopt = 1;
	int fd, ret;

	memset(s, 0, sizeof(*s));

	if (family == AF_INET) {
		serv_addr = (struct sockaddr *)&s->udp4;
		s->udp4.sin_addr = saddr ? saddr->addr :
				   (struct in_addr) {
					.s_addr = htonl(INADDR_ANY),
				   };
		s->udp4.sin_port = htons(port);
		s->udp4.sin_family = AF_INET;
		s->addr_size = sizeof(struct sockaddr_in);
	} else {
		serv_addr = (struct sockaddr *)&s->udp6;
		s->udp6.sin6_addr = saddr ? saddr->addr6 : in6addr_any;
		s->udp6.sin6_port = htons(port);
		s->udp6.sin6_family = AF_INET6;
		s->addr_size = sizeof(struct sockaddr_in6);
	}

	fd = socket(family, SOCK_DGRAM, 0);
	if (fd < 0) {
		perror("socket");
		return fd;
	}

	/* avoid EADDRINUSE error on bind() */
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(int));
	if (ret) {
		perror("setsockopt() SO_REUSEADDR");
		close(fd);
		return ret;
	}

	if (if_name) {
		/* Bind to device, required for IPv6 link-local addresses */
		ret = setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, if_name,
				 IFNAMSIZ - 1);
		if (ret) {
			perror("setsockopt() SO_BINDTODEVICE");
			close(fd);
			return ret;
		}
	}

	ret = bind(fd, serv_addr, s->addr_size);
	if (ret) {
		perror("bind");
		close(fd);
		return ret;
	}

	s->fd = fd;

	return 0;
}

static int igmp_join_by_saddr(struct sock *s, const struct ip_address *mc,
			      struct ip_address *saddr)
{
	struct ip_mreq mreq = {};
	int fd = s->fd;
	int off = 0;
	int ret;

	memcpy(&mreq.imr_multiaddr, &mc->addr, sizeof(struct in_addr));
	memcpy(&mreq.imr_interface.s_addr, &saddr->addr,
	       sizeof(struct in_addr));

	ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret) {
		perror("setsockopt() IP_ADD_MEMBERSHIP");
		return -1;
	}

	ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &off, sizeof(int));
	if (ret) {
		perror("setsockopt() IP_MULTICAST_LOOP");
		return -1;
	}

	return 0;
}

static int igmp_join_by_if_name(struct sock *s, const struct ip_address *mc,
				const char *if_name)
{
	struct ip_mreqn mreq = {};
	int fd = s->fd;
	int if_index;
	int off = 0;
	int ret;

	if_index = if_nametoindex(if_name);
	if (!if_index) {
		perror("if_nametoindex");
		return -1;
	}

	memcpy(&mreq.imr_multiaddr, &mc->addr, sizeof(struct in_addr));
	mreq.imr_ifindex = if_index;

	ret = setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	if (ret) {
		perror("setsockopt() IP_ADD_MEMBERSHIP");
		return -1;
	}

	ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &off, sizeof(int));
	if (ret) {
		perror("setsockopt() IP_MULTICAST_LOOP");
		return -1;
	}

	return 0;
}

static int mld_join(struct sock *s, const struct ip_address *mc,
		    const char *if_name)
{
	struct ipv6_mreq mreq = {};
	int if_index, off = 0;
	int fd = s->fd;
	int ret;

	if_index = if_nametoindex(if_name);
	if (!if_index) {
		perror("if_nametoindex");
		return -1;
	}

	memcpy(&mreq.ipv6mr_multiaddr, &mc->addr6, sizeof(struct in6_addr));
	mreq.ipv6mr_interface = if_index;
	ret = setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &mreq,
			 sizeof(mreq));
	if (ret) {
		perror("setsockopt IPV6_ADD_MEMBERSHIP");
		return -1;
	}

	ret = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &off,
			 sizeof(int));
	if (ret) {
		perror("setsockopt IPV6_MULTICAST_LOOP");
		return -1;
	}

	return 0;
}

int mc_join(struct sock *s, const struct ip_address *mc, const char *if_name,
	    int num_saddrs, struct ip_address *saddrs)
{
	int i, ret;

	if (if_name) {
		switch (mc->family) {
		case AF_INET:
			return igmp_join_by_if_name(s, mc, if_name);
		case AF_INET6:
			return mld_join(s, mc, if_name);
		default:
			return -1;
		}
	}

	if (!num_saddrs) {		/* single interface */
		struct ip_address saddr = {
			.family = AF_INET,
			.addr.s_addr = INADDR_ANY,
		};

		return igmp_join_by_saddr(s, mc, &saddr);
	}

	for (i = 0; i < num_saddrs; i++) {
		ret = igmp_join_by_saddr(s, mc, &saddrs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int igmp_set_ttl(int fd, int ttl)
{
	int ret;

	ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(int));
	if (ret)
		perror("setsockopt() IP_MULTICAST_TTL");

	return ret;
}

static int mld_set_hop_limit(int fd, int limit)
{
	int ret;

	ret = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &limit,
			 sizeof(int));
	if (ret)
		perror("setsockopt() IPV6_MULTICAST_HOPS");

	return ret;
}

int mc_set_hop_limit(struct sock *s, int limit)
{
	switch (s->addr_size) {
	case sizeof(struct sockaddr_in):
		return igmp_set_ttl(s->fd, limit);
	case sizeof(struct sockaddr_in6):
		return mld_set_hop_limit(s->fd, limit);
	default:
		return -1;
	}
}

int mc_recv(struct sock *s, void *buf, size_t len, struct sock *from)
{
	from->addr_size = sizeof(struct sockaddr_in6);

	return recvfrom(s->fd, buf, len, 0, (struct sockaddr *)&(from->udp6),
			&from->addr_size);
}

int mc_send(struct sock *s, struct sock *to, void *buf, size_t len)
{
	return sendto(s->fd, buf, len, 0, (struct sockaddr *)&(to->udp4),
		      s->addr_size);
}

int socket_get_port(const struct sock *s)
{
	switch (s->addr_size) {
	case sizeof(struct sockaddr_in):
		return ntohs(s->udp4.sin_port);
	case sizeof(struct sockaddr_in6):
		return ntohs(s->udp6.sin6_port);
	default:
		return 0;
	}
}

int socket_set_loopback(struct sock *s, int loop)
{
	int fd = s->fd;
	int ret;

	switch (s->addr_size) {
	case sizeof(struct sockaddr_in):
		ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
				 sizeof(int));
		if (ret)
			perror("setsockopt IP_MULTICAST_LOOP");
		break;
	case sizeof(struct sockaddr_in6):
		ret = setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, &loop,
				 sizeof(int));
		if (ret)
			perror("setsockopt IPV6_MULTICAST_LOOP");
		break;
	default:
		return 0;
	}

	return ret;
}
