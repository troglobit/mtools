# mtools

This is a cleaned up version of mtools from the [University of Virginia's
Multimedia Networks Group][1], with added IPv6 and optional SSM support.

The tools [`msend(8)`](msend(8).md) and [`mreceive(8)`](mreceive(8).md)
can be particulary useful when debugging multicast setups.

> Remember, when routing multicast, always check the TTL!

## NAME

* `msend` - send UDP messages to a multicast group
* `mreceive` - receive UDP multicast messages and display them

## SYNOPSIS

	msend [-46hnqv] [-c num] [-g group] [-p port] [-join] [-t TTL] [-i address]
	      [-I interface] [-P period] [-text "text"]
	mreceive [-46hnqv] [-s source ] [-g group] [-p port] [-i ip] ... [-i ip]
	      [-I interface]

## DESCRIPTION

`msend` continuously sends UDP packets to the multicast group specified
by the `-g` and `-p` options.

`mreceive` joins a multicast group specified by the `-g` and `-p`
options, then receives and displays the multicast packets sent to this
group:port combination by the `msend` command.

## OPTIONS

- `-4`

  Select IPv4 test group, use with `-I` when not using `-i` or any
  group.  This is the default.  See `-6` for an example.

- `-6`

  Select IPv6 test group, use with `-I` when not using `-i` or any
  group.  Example:

        $ msend -6 -I eth0
        Now sending to multicast group: [ff2e::1]:4444
        Sending msg 1, TTL 1, to [ff2e::1]:4444:
        ...

- `-c NUM`

  Number of packets to send, default: unlimited.

* `-s SOURCE`

  Source filtering of multicast UDP traffic, a.k.a., source-specific
  multicast (SSM).  A single IPv4/IPv6 address can be given atm.  By
  default, no source filtering is done, mtools default to ASM.

* `-g GROUP`

  Specify the IP multicast group address to which packets are sent, or
  received.  The default group is 224.1.1.1 for IPv4 and ff2e::1 for
  IPv6.

* `-p PORT`

  Specify the UDP port number used by the multicast group.  The default
  port number is 4444.

* `-join`

  Multicast sender will join join the multicast group.  By default, a
  multicast sender does not join the group.

* `-t TTL`

  Specify the TTL (1-255) value in the message sent by `msend`.  You must
  increase this if you want to route the traffic, otherwise the first
  router will drop the packets!  The default value is 1.

* `-i ADDRESS`

  Specify the IP address of the interface to be used to send the packets.
  For `mreceive` one or more interfaces can be given.  The default value
  is `INADDR_ANY` which implies that the default interface selected by
  the system will be used.

* `-I INTERFACE`

  Specify the interface to send on. Can be specified as an alternative
  to `-i`.

* `-P PERIOD`

  Specify the interval in milliseconds between two transmitted packets.
  The default value is 1000 milliseconds.

* `-q`

  Quiet mode, don't print sending or receiving messages.  Errors are
  still printed.

* `-text "text"`

  Specify a message text which is sent as the payload of the packets and
  is displayed by the mreceive(8) command.  The default value is an empty
  string.

* `-n`

  Interpret the contents of the message text as a number instead of a
  string of characters.  Use `mreceive -n` on the other end to interpret
  the message text correctly.

* `-v`

  Print version information.

* `-h`

  Print the command usage.


## BUILD & INSTALL

The build system is a plain Makefile with the following environment
variables to control the build and install process:

 - `CC`: possible to override default compiler, alt. to `CROSS`
 - `CROSS`: set when cross compiling, e.g. `CROSS=aarch64-linux-gnu-`
 - `prefix`: install prefix, default `/usr/local`

**Example:**

    $ make all
    $ make install prefix=/usr


Origin & References
-------------------

This is the continuation of mtools, initially called mSendReceive, written
by Jianping Wang, Yvan Pointurier, and Jörg Liebeherr while at [University
of Virginia's Multimedia Networks Group][1].

The project is maintained by [Joachim Wiberg][3] at [GitHub][2].  Please
file bug reports, clone and send pull requests for bug fixes and proposed
extensions.

[1]: http://www.cs.virginia.edu/~mngroup/software/
[2]: https://github.com/troglobit/mtools
[3]: https://troglobit.com
