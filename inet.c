/*
 * inet.c -- Helper functions for working with IPv4 and IPv6 addresses
 */

#include <errno.h>
#include <arpa/inet.h>

#include "inet.h"


const char *inet_address(const inet_addr_t *ina, char *buf, size_t len)
{
	struct sockaddr_in *sin;

	if (ina->ss_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ina;
		return inet_ntop(AF_INET6, &sin6->sin6_addr, buf, len);
	}

	sin = (struct sockaddr_in *)ina;
	return inet_ntop(AF_INET, &sin->sin_addr, buf, len);
}

socklen_t inet_addrlen(const inet_addr_t *ina)
{
	if (ina->ss_family == AF_INET6)
		return sizeof(struct sockaddr_in6);
	if (ina->ss_family == AF_INET)
		return sizeof(struct sockaddr_in);
	return 0;
}

in_port_t inet_port(const inet_addr_t *ina)
{
	const struct sockaddr_in *sin;

	if (ina->ss_family == AF_INET6) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ina;
		return sin6->sin6_port;
	}

	sin = (struct sockaddr_in *)ina;
	return sin->sin_port;
}

int inet_parse(inet_addr_t *ina, const char *address, in_port_t port)
{
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ina;
	struct sockaddr_in *sin = (struct sockaddr_in *)ina;
	int ret = 0;

	if (!address) {
		errno = EINVAL;
		return -1;
	}

	if (inet_pton(AF_INET6, address, &sin6->sin6_addr) > 0) {
		sin6->sin6_family = AF_INET6;
		sin6->sin6_port = htons(port);
	} else if (inet_pton(AF_INET, address, &sin->sin_addr) > 0) {
		sin->sin_family = AF_INET;
		sin->sin_port = htons(port);
	} else
		ret = -1;

	return ret;
}

/**
 * Local Variables:
 *  c-file-style: "linux"
 *  indent-tabs-mode: t
 * End:
 */
