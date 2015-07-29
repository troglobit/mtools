
NAME
	mreceive - receive UDP multicast messages and display them

SYNOPSIS
	mreceive [-g group] [-p port] [-i ip] ... [-i ip] [-n]
	mreceive [-v|-h]

DESCRIPTION
	Join a multicast group specified by the -g and -p options, receive
	and display the multicast packets sent to this group by the msend 
	command.

	-g	Specify the IP multicast address from which the packets are 
		received. The default group is 224.1.1.1.

	-p	Specify the UDP port number used by the multicast group. The 
		default port number is 4444.

	-i	Specify the IP addresses of one or more interfaces to 
		receive multicast packets. The default value is INADDR_ANY 
		which implies that the default interface selected by the
		system will be used.

	-n	Interpret the contents of the message as a number (messages
		sent with msend -n) instead of a string of characters. It
		should be specified while running msend with -n option.

	-v	Print version information.

	-h	Print the command usage.

SEE ALSO
	msend
