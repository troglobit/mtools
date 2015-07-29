
NAME
	msend - send UDP messages to a multicast group

SYNOPSIS
	msend -g group -p port [-t ttl] [-i ip] [-P period] [-text \"text\"|-n]
	msend -v

DESCRIPTION
	Continuely send UDP packets to a multicast group specified by
	-g and -p options.

	-g	Specify the IP multicast address to which the packets are sent.

	-p	Specify the UDP port number used by the multicast group.

	-t	Specify the ttl value in the message. The default value is 1.

	-i	Specify the IP address of the interface to be used to send the 
		packets. The default value is INADDR_ANY which implies that the
		the default interface selected by the system will be used.

	-P	Specify the interval between two transmitted packets. The 
		default value is 1000 (second).

	-text	Specify a string which is sent as the payload of the packets 
		and is displayed by the mreceive command. The default value is 
		empty string.

	-n	Interpret the contents of the message as a number (messages sent 
		with send -n) instead of a string of characters. It should be 
		specified while running msend with -n option.

	-v	Print version information.

SEE ALSO
	mreceive
