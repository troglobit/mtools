
This is a cleaned up version of mtools from the [University of Virginia's
Multimedia Networks Group][1], with added IPv6 support.

The tools `msend` and `mreceive` can be particulary useful when debugging
multicast setups.

> Remember, when routing multicast, always check the TTL!

## NAME

* `msend` - send UDP messages to a multicast group
* `mreceive` - receive UDP multicast messages and display them

## SYNOPSIS

	msend [-g GROUP] [-p PORT] [-join] [-t TTL] [-i ADDRESS] [-P PERIOD]
	      [-I INTERFACE] [-c NUM] [-text "text" | -n]
	msend [-v|-h]
	mreceive [-g group] [-p port] [-i ip] ... [-i ip] [-I INTERFACE] [-n]
	mreceive [-v|-h]

## DESCRIPTION

`msend` continuously sends UDP packets to the multicast group specified
by the `-g` and `-p` options.

`mreceive` joins a multicast group specified by the `-g` and `-p`
options, then receives and displays the multicast packets sent to this
group:port combination by the `msend` command.

## OPTIONS

- `-c NUM`

  Number of packets to send, default: unlimited.

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

[1]: http://www.cs.virginia.edu/~mngroup/software/
