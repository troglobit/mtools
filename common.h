/*
 * common.h -- Common header for mreceive.c and msend.c
 */
#ifndef _COMMON_H
#define _COMMON_H

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#define logit(fmt, args...) if (verbose) printf(fmt, ##args)

struct ip_address {
	int family;
	union {
		struct in_addr addr;
		struct in6_addr addr6;
	};
};

struct sock {
	socklen_t addr_size;
	union {
		struct sockaddr_in udp4;
		struct sockaddr_in6 udp6;
	};
	int fd;
};

extern int verbose;

int ip_address_parse(const char *string, struct ip_address *ip);
int socket_create(struct sock *s, int family, int port,
		  struct ip_address *saddr, const char *if_name);
int mc_join(struct sock *s, const struct ip_address *mc, const char *if_name,
	    int num_saddrs, struct ip_address *saddrs);
int mc_set_hop_limit(struct sock *s, int limit);
int mc_recv(struct sock *s, void *buf, size_t len, struct sock *from);
int mc_send(struct sock *s, struct sock *to, void *buf, size_t len);
int socket_get_port(const struct sock *s);
int socket_set_loopback(struct sock *s, int loop);

#endif
