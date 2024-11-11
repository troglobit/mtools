MSEND(8) - System Manager's Manual (smm)

# NAME

**msend** - send UDP messages to a multicast group

# SYNOPSIS

**msend**
\[**-46hnvq**]
\[**-c**&nbsp;*NUM*]
\[**-g**&nbsp;*GROUP*]
\[**-join**]
\[**-i**&nbsp;*ADDRESS*]
\[**-I**&nbsp;*INTERFACE*]
\[**-p**&nbsp;*PORT*]
\[**-P**&nbsp;*PERIOD*]
\[**-t**&nbsp;*TTL*]
\[**-text**&nbsp;*'text'*]

# DESCRIPTION

Continuously send UDP packets to the multicast group specified by the
**-g**
and
**-p**
options.

# OPTIONS

**-4**

> Select IPv4 test group, use with
> **-I** *IFACE*
> when not using
> **-i** *ADDRESS*
> or any
> **-g** *GROUP*.
> This is the default.

**-6**

> Select IPv6 test group when not using
> or
> **-g**.
> For an example, see below.

**-c** *NUM*

> Limit number of packets to send, default: unlimited.

**-g** *GROUP*

> Specify the IP multicast group address to which the packets are sent.
> The default group for IPv4 is
> **224.1.1.1**
> and for IPv6
> **ff2e::1**.

**-p** *PORT*

> Specify the UDP port number used by the multicast group.  The default
> port number is
> **4444**.

**-join**

> Multicast sender will join join the multicast group.  By default, a
> multicast sender does not (need to) join the group.

**-t** *TTL*

> Specify the TTL (1-255) value in the message.  You must increase this if
> you want to route the traffic, otherwise the first router will drop the
> packets!  The default value is 1.

**-i** *ADDRESS*

> Specify the IP address of the interface to be used to send the packets.
> The default value is INADDR\_ANY which implies that the default interface
> selected by the system will be used.

**-I** *INTERFACE*

> The interface on which to send.  Can be specified as an alternative to
> **-i**.

**-P** *PERIOD*

> Specify the interval in milliseconds between two transmitted packets.
> The default value is 1000 milliseconds.

**-q**

> Quiet mode, do not log to stdout every time a message is successfully
> sent.  Errors are stil logged.

**-text** *'text'*

> Specify a message text which is sent as the payload of the packets and
> is displayed by the
> mreceive(8)
> command.  The default value is an empty string.

**-n**

> Interpret the contents of the message text as a number instead of a
> string of characters.  Use
> **mreceive**
> **-n**
> on the other end to interpret the message text correctly.

**-v**

> Print version information.

**-h**

> Print the command usage.

# EXAMPLE

	$ msend -6 -I eth0
	Now sending to multicast group: [ff2e::1]:4444
	Sending msg 1, TTL 1, to [ff2e::1]:4444:
	Sending msg 2, TTL 1, to [ff2e::1]:4444:
	Sending msg 3, TTL 1, to [ff2e::1]:4444:
	Sending msg 4, TTL 1, to [ff2e::1]:4444:
	Sending msg 5, TTL 1, to [ff2e::1]:4444:

# SEE ALSO

mreceive(8)

# AUTHORS

**mtools**,
originally called mSendReceive, was written by
Jianping Wang,
Yvan Pointurier,
and
J&#246;rg Liebeherr
while at University of Virginia's Multimedia Networks Group.  It is
currently maintained by
Joachim Wiberg
at
[GitHub](https://github.com/troglobit/mtools).

Debian - November 11, 2024
