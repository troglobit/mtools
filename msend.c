/*
 * msend.c  -- Sends UDP packets to a multicast group
 * 
 * (c)  Jianping Wang, Yvan Pointurier, Jorg Liebeherr, 2002
 *      Multimedia Networks Group, University of Virginia
 *
 * SOURCE CODE RELEASED TO THE PUBLIC DOMAIN
 * 
 * version 2.0 - 5/20/2002 
 * version 2.1 - 12/4/2002  
 * 	By default, msend does not join multicast group. If  -join option is 
 * 	given, msend joins the multicast group. 
 * version 2.2 - 05/17/2003  
 *      Most commandline parameters are assigned default values. The 
 *      usage information is changed according to README_msend.txt
 * 
 * 
 * Based on this public domain program:
 * u_mctest.c            (c) Bob Quinn           2/4/97
 * 
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h> 
#include <sys/time.h>

#define TRUE 1
#define FALSE 0
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif
#define LOOPMAX   20
#define BUFSIZE   1024

char *TEST_ADDR = "224.1.1.1";
int TEST_PORT = 4444;
int TTL_VALUE = 1;
int SLEEP_TIME=1000; 
unsigned long IP = INADDR_ANY; 
int NUM=0; 

int join_flag = 0; /* not join */

typedef struct timerhandler_s {
	int 	s; 
	char    *achOut; 
	int     len; 
	int 	n; 
	struct sockaddr *stTo; 
	int 	addr_size; 
} timerhandler_t; 
timerhandler_t handler_par; 
void timerhandler(); 

void printHelp(void) { 
      printf("msend version 2.2\n\
Usage:  msend [-g group] [-p port] [-join] [-i IP] [-t ttl] [-P period]\n\
	      [-text \"text\"|-n]\n\
	msend [-v|-h]\n\n\
        -g group     Specify the IP multicast address to which the packets are\n\
		     sent. The default group is 224.1.1.1.\n\
        -p port      Specify the UDP port number used by the multicast group.\n\
		     The default port number is 4444.\n\
        -join        Multicast sender will join join the multicast group. By\n\
		     default, a multicast sender does not join the group.\n\
        -t ttl       Specify the ttl value in the message. The default value\n\
		     is 1.\n\
        -i IP        Specify the IP address of the interface to be used to send\n\
		     the packets. The default value is INADDR_ANY which implies\n\
		     that the the default interface selected by the system will\n\
		     be used.\n\
        -P period    Specify the interval in milliseconds between two transmitted\n\
		     packets. The default value is 1000 milliseconds.\n\
        -text \"text\" Specify a string which is sent as the payload of the packets \n\
                     and is displayed by the mreceive command. The default value\n\
                     is empty string.\n\
        -n           Interpret the contents of the message as a number (messages\n\
                     sent with msend -n) instead of a string of characters.\n\ 
        -v           Print version information.\n\
        -h           Print the command usage.\n");
}

int main( int argc, char *argv[])
{
  struct sockaddr_in stLocal, stTo;
  char achOut[BUFSIZE];
  int s, i;
  struct ip_mreq stMreq;
  int iTmp, iRet;
  int ii;
  int addr_size = sizeof(struct sockaddr_in);
  struct itimerval times;
  sigset_t sigset;
  struct sigaction act;
  siginfo_t si; 

/*
  printf("argc = %d\n", argc);
  if( argc < 2 ) {
    printHelp(); 
    return 1;
  }
*/
  strcpy(achOut, ""); 

/* not join */
/*  join_flag = 0; 
*/

  ii = 1;

  if ( ( argc == 2 ) && (strcmp(argv[ii], "-v") == 0) ) {
        printf("msend version 2.2\n");
        return 0;
  }
  if ( ( argc == 2 ) && (strcmp(argv[ii], "-h") == 0) ) {
    	printHelp(); 
        return 0;
  }

  while (ii < argc) {
    if ( strcmp(argv[ii], "-g") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           TEST_ADDR = argv[ii];
           ii++;
	}
    }
    else if ( strcmp(argv[ii], "-p") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           TEST_PORT = atoi(argv[ii]);
           ii++;
	}
    }
    else if ( strcmp(argv[ii], "-join") == 0 ) {
        join_flag++;;
        ii++;
    }
    else if ( strcmp(argv[ii], "-i") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           IP = inet_addr(argv[ii]); 
           ii++; 
	}
    }
    else if ( strcmp(argv[ii], "-t") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           TTL_VALUE = atoi(argv[ii]);
           ii++;
	}
    }
    else if ( strcmp(argv[ii], "-P") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           SLEEP_TIME = atoi(argv[ii]);
           ii++;
	}
    }
    else if ( strcmp(argv[ii], "-n") == 0 ) {
        ii++;
        NUM=1;
        ii++;
    }
    else if ( strcmp(argv[ii], "-text") == 0 ) {
        ii++;
  	if ((ii < argc) && !(strchr(argv[ii],'-'))){
           strcpy(achOut, argv[ii]);
           ii++;
	}
    }
    else {
        printf("wrong parameters!\n\n");
        printHelp();
        return 1;
    }
  }

  /* get a datagram socket */
  s = socket(AF_INET, 
     SOCK_DGRAM, 
     0);
  if (s == INVALID_SOCKET) {
    printf ("socket() failed.\n");
    exit(1);
  }

  /* avoid EADDRINUSE error on bind() */ 
  iTmp = TRUE;
  iRet = setsockopt(s, 
     SOL_SOCKET, 
     SO_REUSEADDR, 
     (char *)&iTmp, 
     sizeof(iTmp));
  if (iRet == SOCKET_ERROR) {
    printf ("setsockopt() SO_REUSEADDR failed.\n");
    exit(1);
  }

  /* name the socket */
  stLocal.sin_family = 	 AF_INET;
  stLocal.sin_addr.s_addr = IP;
  stLocal.sin_port = 	 htons(TEST_PORT);
  iRet = bind(s, (struct sockaddr*) &stLocal, sizeof(stLocal));
  if (iRet == SOCKET_ERROR) {
    printf ("bind() failed.\n");
    exit(1);
  }

  /* join the multicast group. */
  stMreq.imr_multiaddr.s_addr = inet_addr(TEST_ADDR);
  stMreq.imr_interface.s_addr = IP;
  if (join_flag == 1) {
     iRet = setsockopt(s, 
        IPPROTO_IP, 
        IP_ADD_MEMBERSHIP, 
        (char *)&stMreq, 
        sizeof(stMreq));
     if (iRet == SOCKET_ERROR) {
       printf ("setsockopt() IP_ADD_MEMBERSHIP failed.\n");
       exit(1);
     } 
  } 

  /* set TTL to traverse up to multiple routers */
  iTmp = TTL_VALUE;
  iRet = setsockopt(s, 
     IPPROTO_IP, 
     IP_MULTICAST_TTL, 
     (char *)&iTmp, 
     sizeof(iTmp));
  if (iRet == SOCKET_ERROR) {
    printf ("setsockopt() IP_MULTICAST_TTL failed.\n");
    exit(1);
  }

  /* enable loopback */
  iTmp = TRUE;
  iRet = setsockopt(s, 
     IPPROTO_IP, 
     IP_MULTICAST_LOOP, 
     (char *)&iTmp, 
     sizeof(iTmp));
  if (iRet == SOCKET_ERROR) {
    printf ("setsockopt() IP_MULTICAST_LOOP failed.\n");
    exit(1);
  }

  /* assign our destination address */
  stTo.sin_family =      AF_INET;
  stTo.sin_addr.s_addr = inet_addr(TEST_ADDR);
  stTo.sin_port =        htons(TEST_PORT);
  printf ("Now sending to multicast group: %s\n", TEST_ADDR);

  SLEEP_TIME*=1000; /* convert to microsecond */
  if (SLEEP_TIME>0) {
    /* block SIGALRM */
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
	
    /* set up handler for SIGALRM */
    act.sa_handler = &timerhandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGALRM, &act, NULL);
    /*
     * set up interval timer
     */
    times.it_value.tv_sec     = 0; /* wait a bit for system to "stabilize"  */
    times.it_value.tv_usec    = 1; /* tv_sec or tv_usec cannot be both zero */
    times.it_interval.tv_sec  = (time_t)(SLEEP_TIME/1000000); 
    times.it_interval.tv_usec = (long)  (SLEEP_TIME%1000000); 
    setitimer(ITIMER_REAL, &times, NULL); 
	
    handler_par.s 		= s;  
    handler_par.achOut 	= achOut; 
    handler_par.len 	= strlen(achOut)+1; 
    handler_par.n 		= 0; 
    handler_par.stTo 	= (struct sockaddr*)&stTo; 
    handler_par.addr_size	= addr_size; 
	
    /* now wait for the alarms */
    sigemptyset(&sigset);
    for(;;) {
      sigsuspend(&sigset);
    }
    return; 
  } else {
    for (i=0;i<10;i++) {
      int addr_size = sizeof(struct sockaddr_in);
			
      if (NUM) {
	achOut[3] = (unsigned char)(i>>24); 
	achOut[2] = (unsigned char)(i>>16); 
	achOut[1] = (unsigned char)(i>>8); 
	achOut[0] = (unsigned char)(i); 
	printf("Send out msg %d to %s:%d\n", i, TEST_ADDR, TEST_PORT);
      } else {
	printf("Send out msg %d to %s:%d: %s\n", i, TEST_ADDR, TEST_PORT, achOut);
      }
			
      iRet = sendto(s, 
		    achOut, 
		    (NUM?4:strlen(achOut)+1), 
		    0,
		    (struct sockaddr*)&stTo, 
		    addr_size);
      if (iRet < 0) {
	printf ("sendto() failed.\n");
	exit(1);
      }
    } /* end for(;;) */
  } 

  return 0; 
} /* end main() */  

void timerhandler(int sig, siginfo_t *siginfo, void  *context) {
    int iRet; 
    static long iCounter = 1;

    if (NUM) {
      handler_par.achOut = (char *)(&iCounter); 
      handler_par.len    = sizeof(iCounter); 
      printf("Send out msg %d to %s:%d\n", iCounter, TEST_ADDR, TEST_PORT);
    } else {
      printf("Send out msg %d to %s:%d: %s\n", iCounter, TEST_ADDR, TEST_PORT, handler_par.achOut);
    }
    iRet = sendto( handler_par.s, 
      handler_par.achOut, 
      handler_par.len, 
      handler_par.n, 
      handler_par.stTo, 
      handler_par.addr_size);
    if (iRet < 0) {
      printf ("sendto() failed.\n");
      exit(1);
    }
    iCounter++;
    return; 
}
