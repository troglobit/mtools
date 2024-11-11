/*
 * inet.h -- Helper functions for working with IPv4 and IPv6 addresses
 */

#ifndef MTOOLS_INET_H_
#define MTOOLS_INET_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>

#define INET_ADDRSTR_LEN  INET6_ADDRSTRLEN

typedef struct sockaddr_storage inet_addr_t;

const char *inet_address (const inet_addr_t *ina, char *buf, size_t len);
socklen_t   inet_addrlen (const inet_addr_t *ina);
in_port_t   inet_port    (const inet_addr_t *ina);

int         inet_parse   (inet_addr_t *ina, const char *address, in_port_t port);

#endif /* MTOOLS_INET_H_ */

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
