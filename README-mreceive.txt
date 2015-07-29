
NAME
	mreceive - receive UDP multicast messages and display them

SYNOPSIS
	mreceive -g group -p port [-i ip] ... [-i ip] [-n]
	mreceive -v

DESCRIPTION
	Join a multicast group specified by the -g and -p options, receive
	and display the multicast packets sent to this group by the msend 
	command.

	-g	Specify the IP multicast address from which the packets are 
		received.

	-p	Specify the UDP port number used by the multicast group.

	-i	Specify the IP addresses of one or more interfaces to 
		receive multicast packets.

	-n	Interpret the contents of the message as a number (messages
		sent with send -n) instead of a string of characters.

        -v      Print version information.

SEE ALSO
	msend
