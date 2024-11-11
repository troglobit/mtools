MSEND(8) - System Manager's Manual (smm)

# NAME

**mreceive** - receive UDP multicast messages and display them

# SYNOPSIS

**mreceive**
\[**-46hnvq**]
\[**-g**&nbsp;*GROUP*]
\[**-i**&nbsp;*ADDRESS*]
\[...]
\[**-i**&nbsp;*ADDRESS*]
\[**-I**&nbsp;*INTERFACE*]
\[**-p**&nbsp;*PORT*]
\[**-s**&nbsp;*ADDRESS*]

# DESCRIPTION

Join a multicast group specified by the
**-g**
and
**-p**
options, then receive and display the multicast packets sent to this
group:port by the
msend(8)
command.

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

**-s** *ADDRESS*

> Optional source IP address for source-specific filtering (SSM).  By
> default,
> **mreceive**
> runs in any-source multicast (ASM) mode.

**-g** *GROUP*

> Specify the IP multicast address from which the packets are received.
> The default group is
> **224.1.1.1**.

**-p** *PORT*

> Specify the UDP port number used by the multicast group.  The default
> port number is
> **4444**.

**-q**

> Quiet mode, do not log to stdout every time a message is received.
> Errors are stil logged.

**-i** *ADDRESS*

> Specify the IP addresses of one or more interfaces to receive multicast
> packets.  The default value is
> **INADDR\_ANY**
> which implies that the default interface selected by the system will be
> used.

**-I** *INTERFACE*

> The interface on which to receive.  Can be specified as an alternative
> to
> **-i**.

**-n**

> Interpret the contents of the message as a number instead of a string of
> characters.  This option should be given when running
> **msend**
> with the
> **-n**
> option.

**-v**

> Print version information.

**-h**

> Print the command usage.

# EXAMPLE

	$ mreceive -6 -I eth0
	Now receiving from multicast group: [ff2e::1]:4444
	Receive msg 1 from [2001:9b0:214:3500::c2e]:37801:
	Receive msg 2 from [2001:9b0:214:3500::c2e]:37801:
	Receive msg 3 from [2001:9b0:214:3500::c2e]:37801:
	Receive msg 4 from [2001:9b0:214:3500::c2e]:37801:
	^C

# SEE ALSO

msend(8)

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
