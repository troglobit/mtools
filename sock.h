/*
 * sock.h -- Helper functions for working with multicast sockets
 */

#ifndef MTOOLS_SOCK_H_
#define MTOOLS_SOCK_H_

#include "inet.h"

int         sock_create  (inet_addr_t *ina, const char *ifname);
int         sock_family  (int sd);
int         sock_mc_loop (int sd, int loop);
int         sock_mc_ttl  (int sd, int ttl);

int         sock_mc_join (int sd, const inet_addr_t *source, const inet_addr_t *group,
			  const char *ifname, int num_ifaddrs, inet_addr_t *ifaddrs);

#endif /* MTOOLS_SOCK_H_ */

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
