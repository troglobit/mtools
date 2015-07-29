
NAME
	msend - send UDP messages to a multicast group

SYNOPSIS
	msend [-g group] [-p port] [-join] [-t ttl] [-i ip] [-P period] 
	      [-text "text"|-n]
	msend [-v|-h]

DESCRIPTION
	Continuely send UDP packets to a multicast group specified by -g 
	and -p options.

	-g	Specify the IP multicast address to which the packets are
		sent. The default group is 224.1.1.1.

	-p	Specify the UDP port number used by the multicast group.
		The default port number is 4444.

	-join	Multicast sender will join join the multicast group. By
		default, a multicast sender does not join the group.

	-t	Specify the ttl value in the message. The default value
		is 1.

	-i	Specify the IP address of the interface to be used to send
		the packets. The default value is INADDR_ANY which implies 
		that the default interface selected by the system will be 
		used.

	-P	Specify the interval in milliseconds between two transmitted
		packets. The default value is 1000 milliseconds.

	-text	Specify a string which is sent as the payload of the packets 
		and is displayed by the mreceive command. The default value
		is empty string.

	-n	Interpret the contents of the message as a number (messages
		sent with msend -n) instead of a string of characters. 

	-v	Print version information.

	-h	Print the command usage.

SEE ALSO
	mreceive
