/*
 *	T T C P . C
 *
 * Test TCP connection.  Makes a connection on port 5001
 * and transfers fabricated buffers or data copied from stdin.
 *
 * Usable on 4.2, 4.3, and 4.1a systems by defining one of
 * BSD42 BSD43 (BSD41a)
 * Machines using System V with BSD sockets should define SYSV.
 *
 * Modified for operation under 4.2BSD, 18 Dec 84
 *      T.C. Slattery, USNA
 * Minor improvements, Mike Muuss and Terry Slattery, 16-Oct-85.
 * Modified in 1989 at Silicon Graphics, Inc.
 *	catch SIGPIPE to be able to print stats when receiver has died 
 *	for tcp, don't look for sentinel during reads to allow small transfers
 *	increased default buffer size to 8K, nbuf to 2K to transfer 16MB
 *	moved default port to 5001, beyond IPPORT_USERRESERVED
 *	make sinkmode default because it is more popular, 
 *		-s now means don't sink/source 
 *	count number of read/write system calls to see effects of 
 *		blocking from full socket buffers
 *	for tcp, -D option turns off buffered writes (sets TCP_NODELAY sockopt)
 *	buffer alignment options, -A and -O
 *	print stats in a format that's a bit easier to use with grep & awk
 *	for SYSV, mimic BSD routines to use most of the existing timing code
 *
 * Distribution Status -
 *      Public Domain.  Distribution Unlimited.
 */
/* Modified by Yvan Pointurier (yp9x), University of Virginia - May 2002 */
#ifndef lint
static char RCSid[] = "ttcp.c $Revision: 1.4 $";
#endif

#define BSD43
/* #define BSD42 */
/* #define BSD41a */
#if defined(sgi)
#define SYSV
#endif

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/time.h>		/* struct timeval */

#if defined(SYSV)
#include <sys/times.h>
#include <sys/param.h>
struct rusage {
    struct timeval ru_utime, ru_stime;
};
#define RUSAGE_SELF 0
#else
#include <sys/resource.h>
#endif

struct sockaddr_in sinme;
struct sockaddr_in sinhim;
struct sockaddr_in frominet;

int domain, fromlen;
int fd;				/* fd of network socket */

int buflen = 8 * 1024;		/* length of buffer */
char *buf;			/* ptr to dynamic buffer */
int nbuf = 8 * 1024;		/* number of buffers to send in sinkmode */

int bufoffset = 0;		/* align buffer to this */
int bufalign = 16*1024;		/* modulo this */

int udp = 0;			/* 0 = tcp, !0 = udp */
int ttl=100;                    /* for multicast only */
int options = 0;		/* socket options */
int one = 1;                    /* for 4.3 BSD style setsockopt() */
short port = 5001;		/* TCP port number */
char *host;			/* ptr to name of host */
int trans;			/* 0=receive, !0=transmit mode */
int sinkmode = 1;		/* 0=normal I/O, !0=sink/source mode */
int verbose = 0;		/* 0=print basic info, 1=print cpu rate, proc
				 * resource usage. */
int nodelay = 0;		/* set TCP_NODELAY socket option */
int b_flag = 0;			/* use mread() */
int mcast = 0;                  /* 1 = multicast reception, 0 = unicast */
char *mgroup;                   /* multicast group address */
/*#define MAXPAK 32768           */ /* max # of packets received */ 
/*int rcvBytesArray[MAXPAK];   */
/*double rcvTimeArray[MAXPAK]; */
int rcvIndex = 0;               /* index in the arrays */
long rate = 0;                  /* sending rate */

struct hostent *addr;
extern int errno;

char Usage[] = "\
Usage: ttcp -t [-options] host [ < in ]\n\
       ttcp -r [-options > out]\n\
Common options:\n\
	-l##	length of bufs read from or written to network (default 8192)\n\
	-u	use UDP instead of TCP\n\
	-p##	port number to send to or listen at (default 5001)\n\
	-s	-t: don't source a pattern to network, get data from stdin\n\
		-r: don't sink (discard), print data on stdout\n\
	-A	align the start of buffers to this modulus (default 16384)\n\
	-O	start buffers at this offset from the modulus (default 0)\n\
	-v	verbose: print more statistics\n\
	-d	set SO_DEBUG socket option\n\
Options specific to -t:\n\
	-n##	number of source bufs written to network (default 8192)\n\
	-D	don't buffer TCP writes (sets TCP_NODELAY socket option)\n\
	-iTTL   sets the ttl of the packets (multicast packets)\n\
        -R##    slows down sending rate (higher values mean slower rates\n\
Options specific to -r:\n\
	-B	for -s, only output full blocks as specified by -l (for TAR)\n\
        -m IP   bind to a multicast group designated by its IP address\n\
";	

char stats[128];
unsigned long nbytes;		/* bytes on net */
unsigned long numCalls;		/* # of I/O system calls */

void prep_timer();
double read_timer();
double cput, realt;		/* user, real time (seconds) */

void
sigpipe()
{
}

main(argc,argv)
int argc;
char **argv;
{
	unsigned long addr_tmp;

	if (argc < 2) goto usage;

	argv++; argc--;
	while( argc>0 && argv[0][0] == '-' )  {
		switch (argv[0][1]) {

		case 'B':
			b_flag = 1;
			break;
		case 't':
			trans = 1;
			break;
		case 'r':
			trans = 0;
			break;
		case 'd':
			options |= SO_DEBUG;
			break;
		case 'D':
			nodelay = 1;
			break;
		case 'n':
			nbuf = atoi(&argv[0][2]);
			break;
		case 'l':
			buflen = atoi(&argv[0][2]);
			break;
		case 's':
			sinkmode = 0;	/* sink/source data */
			break;
		case 'p':
			port = atoi(&argv[0][2]);
			break;
		case 'u':
			udp = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'A':
			bufalign = atoi(&argv[0][2]);
			break;
		case 'O':
			bufoffset = atoi(&argv[0][2]);
			break;
		case 'i':
			ttl = atoi(&argv[0][2]);
			break;
		case 'R':
			rate = atol(&argv[0][2]);
			break;
                case 'm': 
                        mcast = 1; 
                        argv++; argc--; 
                        if (argc>0) mgroup = argv[0]; else goto usage; 
                        break; 
		default:
			goto usage;
		}
		argv++; argc--;
	}
	if(trans)  {
		/* xmitr */
		if (argc != 1) goto usage;
		bzero((char *)&sinhim, sizeof(sinhim));
		host = argv[0];
		if (atoi(host) > 0 )  {
			/* Numeric */
			sinhim.sin_family = AF_INET;
#if defined(cray)
			addr_tmp = inet_addr(host);
			sinhim.sin_addr = addr_tmp;
#else
			sinhim.sin_addr.s_addr = inet_addr(host);
#endif
		} else {
			if ((addr=gethostbyname(host)) == NULL)
				err("bad hostname");
			sinhim.sin_family = addr->h_addrtype;
			bcopy(addr->h_addr,(char*)&addr_tmp, addr->h_length);
#if defined(cray)
			sinhim.sin_addr = addr_tmp;
#else
			sinhim.sin_addr.s_addr = addr_tmp;
#endif cray
		}
		sinhim.sin_port = htons(port);
		sinme.sin_port = 0;		/* free choice */
	} else {
		/* rcvr */
		sinme.sin_port =  htons(port);
	}


	if (udp && buflen < 5) {
	    buflen = 5;		/* send more than the sentinel size */
	}

	if ( (buf = (char *)malloc(buflen+bufalign)) == (char *)NULL)
		err("malloc");
	if (bufalign != 0)
		buf +=(bufalign - ((int)buf % bufalign) + bufoffset) % bufalign;

	if (trans) {
	    fprintf(stdout,
	    "#ttcp-t: buflen=%d, nbuf=%d, align=%d/+%d, port=%d  %s  -> %s\n",
		buflen, nbuf, bufalign, bufoffset, port,
		udp?"udp":"tcp",
		argv[0]);
	} else {
	    fprintf(stdout,
	    "#ttcp-r: buflen=%d, nbuf=%d, align=%d/+%d, port=%d  %s\n",
		buflen, nbuf, bufalign, bufoffset, port,
		udp?"udp":"tcp");
	}

	if ((fd = socket(AF_INET, udp?SOCK_DGRAM:SOCK_STREAM, 0)) < 0)
		err("socket");
	mes("socket");

        if (bind(fd, &sinme, sizeof(sinme)) < 0)
		err("bind");

        if (mcast) {
          struct ip_mreq stMreq;
          stMreq.imr_multiaddr.s_addr = inet_addr(mgroup);
          stMreq.imr_interface.s_addr = INADDR_ANY;
          if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&stMreq, sizeof(stMreq)) <0)
                err("multicast group join"); 
          else fprintf(stdout, "#Joined group %s\n", mgroup); 
        }

	if (!udp)  {
	    signal(SIGPIPE, sigpipe);
	    if (trans) {
		/* We are the client if transmitting */
		if(options)  {
#if defined(BSD42)
			if( setsockopt(fd, SOL_SOCKET, options, 0, 0) < 0)
#else BSD43
			if( setsockopt(fd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
#endif
				err("setsockopt");
		}
		if (nodelay) {
			struct protoent *p;
			p = getprotobyname("tcp");
			if( p && setsockopt(fd, p->p_proto, TCP_NODELAY, 
			    &one, sizeof(one)) < 0)
				err("setsockopt: nodelay");
			mes("nodelay");
		}
		if(connect(fd, &sinhim, sizeof(sinhim) ) < 0)
			err("connect");
		mes("connect");
	    } else {
		/* otherwise, we are the server and 
	         * should listen for the connections
	         */
		listen(fd,0);   /* allow a queue of 0 */
		if(options)  {
#if defined(BSD42)
			if( setsockopt(fd, SOL_SOCKET, options, 0, 0) < 0)
#else BSD43
			if( setsockopt(fd, SOL_SOCKET, options, &one, sizeof(one)) < 0)
#endif
				err("setsockopt");
		}
		fromlen = sizeof(frominet);
		domain = AF_INET;
		if((fd=accept(fd, &frominet, &fromlen) ) < 0)
			err("accept");
		{ struct sockaddr_in peer;
		  int peerlen = sizeof(peer);
		  if (getpeername(fd, (struct sockaddr_in *) &peer, 
				&peerlen) < 0) {
			err("getpeername");
		  }
		  fprintf(stderr,"ttcp-r: accept from %s\n", 
			inet_ntoa(peer.sin_addr));
		}
	    }
	}

	prep_timer();
	errno = 0;

	if (sinkmode) {      
		register int cnt;

                if (udp) {
                  int ret=0; 

                  if ( (ntohl(sinhim.sin_addr.s_addr))>>28 == 0xe) {
                    ret = setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(int)); 
                    if (ret==-1) fprintf(stderr, "Error while setting TTL\n"); else fprintf(stdout, "#TTL set to %d\n", ttl);  
                  }
                }
		if (trans)  {
			pattern( buf, buflen );
			if(udp)  (void)Nwrite( fd, buf, 4 ); /* rcvr start */
			while (nbuf-- && Nwrite(fd,buf,buflen) == buflen) 
				nbytes += buflen;
			if(udp)  (void)Nwrite( fd, buf, 4 ); /* rcvr end */
		} else {
			if (udp) {
			    while ((cnt=Nread(fd,buf,buflen)) > 0)  {
/*
//yp9x: allow for duplicate starting packet
				    static int going = 2; 
				    if( cnt <= 4 )  {
					    if( going == 0) 
						    break; 
					    going--;
					    prep_timer();
*/

/*
//original version
				    static int going = 0; 
				    if( cnt <= 4 )  {
					    if( going ) 
						    break; 
					    going=1; 
					    prep_timer();

*/
				    static int going = 0; 
				    if( cnt <= 4 )  {
					    if( going ) 
						    break; 
					    going=1; 
					    prep_timer();

				    } else {
					    nbytes += cnt;
					    read_timer(stats,sizeof(stats)); 
//					    fprintf(stdout, "%f\t%d\n", realt, nbytes); 
					    rcvIndex++; 
				    }
			    }
			} else {
			    while ((cnt=Nread(fd,buf,buflen)) > 0)  {
				    nbytes += cnt;
			    }
			}
		}
	} else { 
		register int cnt;

		if (trans)  {
			while((cnt=read(0,buf,buflen)) > 0 &&
			    Nwrite(fd,buf,cnt) == cnt)
				nbytes += cnt;
		}  else  {
			while((cnt=Nread(fd,buf,buflen)) > 0 &&
			    write(1,buf,cnt) == cnt)
				nbytes += cnt;
		}
	}
	if(errno) err("IO");
	read_timer(stats,sizeof(stats));
	if(udp&&trans)  {
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */ /* yp9x */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */ /* yp9x */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */ /* yp9x */
		(void)Nwrite( fd, buf, 4 ); /* rcvr end */ /* yp9x */
	}
	if( cput <= 0.0 )  cput = 0.001;
	if( realt <= 0.0 )  realt = 0.001;
	fprintf(stdout,
		"#ttcp%s: %ld bytes in %.2f real seconds = %.2f KB/sec +++\n",
		trans?"-t":"-r",
		nbytes, realt, ((double)nbytes)/realt/1024 );
	if (verbose) {
	    fprintf(stdout,
		"#ttcp%s: %ld bytes in %.2f CPU seconds = %.2f KB/cpu sec\n",
		trans?"-t":"-r",
		nbytes, cput, ((double)nbytes)/cput/1024 );
	}
	fprintf(stdout,
		"#ttcp%s: %d I/O calls, msec/call = %.2f, calls/sec = %.2f\n",
		trans?"-t":"-r",
		numCalls,
		1024.0 * realt/((double)numCalls),
		((double)numCalls)/realt);
	fprintf(stdout,"#ttcp%s: %s\n", trans?"-t":"-r", stats);
	if (verbose) {
	    fprintf(stdout,
		"#ttcp%s: buffer address %#x\n",
		trans?"-t":"-r",
		buf);
	}
/*	printStats(); */
	exit(0);

usage:
	fprintf(stderr,Usage);
	exit(1);
}

err(s)
char *s;
{
	fprintf(stderr,"ttcp%s: ", trans?"-t":"-r");
	perror(s);
	fprintf(stderr,"errno=%d\n",errno);
	exit(1);
}

mes(s)
char *s;
{
	fprintf(stderr,"ttcp%s: %s\n", trans?"-t":"-r", s);
}

pattern( cp, cnt )
register char *cp;
register int cnt;
{
	register char c;
	c = 0;
	while( cnt-- > 0 )  {
		while( !isprint((c&0x7F)) )  c++;
		*cp++ = (c++&0x7F);
	}
}


static struct	timeval time0;	/* Time at which timing started */
static struct	rusage ru0;	/* Resource utilization at the start */

static void prusage();
static void tvadd();
static void tvsub();
static void psecs();

#if defined(SYSV)
/*ARGSUSED*/
static
getrusage(ignored, ru)
    int ignored;
    register struct rusage *ru;
{
    struct tms buf;

    times(&buf);

    /* Assumption: HZ <= 2147 (LONG_MAX/1000000) */
    ru->ru_stime.tv_sec  = buf.tms_stime / HZ;
    ru->ru_stime.tv_usec = ((buf.tms_stime % HZ) * 1000000) / HZ;
    ru->ru_utime.tv_sec  = buf.tms_utime / HZ;
    ru->ru_utime.tv_usec = ((buf.tms_utime % HZ) * 1000000) / HZ;
}

#if !defined(sgi)
/*ARGSUSED*/
static 
gettimeofday(tp, zp)
    struct timeval *tp;
    struct timezone *zp;
{
    tp->tv_sec = time(0);
    tp->tv_usec = 0;
}
#endif
#endif SYSV

/*
 *			P R E P _ T I M E R
 */
void
prep_timer()
{
	gettimeofday(&time0, (struct timezone *)0);
	getrusage(RUSAGE_SELF, &ru0);
}

/*
 *			R E A D _ T I M E R
 * 
 */
double
read_timer(char *str,int len)
{
	struct timeval timedol;
	struct rusage ru1;
	struct timeval td;
	struct timeval tend, tstart;
	char line[132];

	getrusage(RUSAGE_SELF, &ru1);
	gettimeofday(&timedol, (struct timezone *)0);
	prusage(&ru0, &ru1, &timedol, &time0, line);
	(void)strncpy( str, line, len );

	/* Get real time */
	tvsub( &td, &timedol, &time0 );
	realt = td.tv_sec + ((double)td.tv_usec) / 1000000;

	/* Get CPU time (user+sys) */
	tvadd( &tend, &ru1.ru_utime, &ru1.ru_stime );
	tvadd( &tstart, &ru0.ru_utime, &ru0.ru_stime );
	tvsub( &td, &tend, &tstart );
	cput = td.tv_sec + ((double)td.tv_usec) / 1000000;
	if( cput < 0.00001 )  cput = 0.00001;
	return( cput );
}

static void
prusage(r0, r1, e, b, outp)
	register struct rusage *r0, *r1;
	struct timeval *e, *b;
	char *outp;
{
	struct timeval tdiff;
	register time_t t;
	register char *cp;
	register int i;
	int ms;

	t = (r1->ru_utime.tv_sec-r0->ru_utime.tv_sec)*100+
	    (r1->ru_utime.tv_usec-r0->ru_utime.tv_usec)/10000+
	    (r1->ru_stime.tv_sec-r0->ru_stime.tv_sec)*100+
	    (r1->ru_stime.tv_usec-r0->ru_stime.tv_usec)/10000;
	ms =  (e->tv_sec-b->tv_sec)*100 + (e->tv_usec-b->tv_usec)/10000;

#define END(x)	{while(*x) x++;}
#if defined(SYSV)
	cp = "%Uuser %Ssys %Ereal %P";
#else
	cp = "%Uuser %Ssys %Ereal %P %Xi+%Dd %Mmaxrss %F+%Rpf %Ccsw";
#endif
	for (; *cp; cp++)  {
		if (*cp != '%')
			*outp++ = *cp;
		else if (cp[1]) switch(*++cp) {

		case 'U':
			tvsub(&tdiff, &r1->ru_utime, &r0->ru_utime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'S':
			tvsub(&tdiff, &r1->ru_stime, &r0->ru_stime);
			sprintf(outp,"%d.%01d", tdiff.tv_sec, tdiff.tv_usec/100000);
			END(outp);
			break;

		case 'E':
			psecs(ms / 100, outp);
			END(outp);
			break;

		case 'P':
			sprintf(outp,"%d%%", (int) (t*100 / ((ms ? ms : 1))));
			END(outp);
			break;

#if !defined(SYSV)
		case 'W':
			i = r1->ru_nswap - r0->ru_nswap;
			sprintf(outp,"%d", i);
			END(outp);
			break;

		case 'X':
			sprintf(outp,"%d", t == 0 ? 0 : (r1->ru_ixrss-r0->ru_ixrss)/t);
			END(outp);
			break;

		case 'D':
			sprintf(outp,"%d", t == 0 ? 0 :
			    (r1->ru_idrss+r1->ru_isrss-(r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'K':
			sprintf(outp,"%d", t == 0 ? 0 :
			    ((r1->ru_ixrss+r1->ru_isrss+r1->ru_idrss) -
			    (r0->ru_ixrss+r0->ru_idrss+r0->ru_isrss))/t);
			END(outp);
			break;

		case 'M':
			sprintf(outp,"%d", r1->ru_maxrss/2);
			END(outp);
			break;

		case 'F':
			sprintf(outp,"%d", r1->ru_majflt-r0->ru_majflt);
			END(outp);
			break;

		case 'R':
			sprintf(outp,"%d", r1->ru_minflt-r0->ru_minflt);
			END(outp);
			break;

		case 'I':
			sprintf(outp,"%d", r1->ru_inblock-r0->ru_inblock);
			END(outp);
			break;

		case 'O':
			sprintf(outp,"%d", r1->ru_oublock-r0->ru_oublock);
			END(outp);
			break;
		case 'C':
			sprintf(outp,"%d+%d", r1->ru_nvcsw-r0->ru_nvcsw,
				r1->ru_nivcsw-r0->ru_nivcsw );
			END(outp);
			break;
#endif !SYSV
		}
	}
	*outp = '\0';
}

static void
tvadd(tsum, t0, t1)
	struct timeval *tsum, *t0, *t1;
{

	tsum->tv_sec = t0->tv_sec + t1->tv_sec;
	tsum->tv_usec = t0->tv_usec + t1->tv_usec;
	if (tsum->tv_usec > 1000000)
		tsum->tv_sec++, tsum->tv_usec -= 1000000;
}

static void
tvsub(tdiff, t1, t0)
	struct timeval *tdiff, *t1, *t0;
{

	tdiff->tv_sec = t1->tv_sec - t0->tv_sec;
	tdiff->tv_usec = t1->tv_usec - t0->tv_usec;
	if (tdiff->tv_usec < 0)
		tdiff->tv_sec--, tdiff->tv_usec += 1000000;
}

static void
psecs(l,cp)
long l;
register char *cp;
{
	register int i;

	i = l / 3600;
	if (i) {
		sprintf(cp,"%d:", i);
		END(cp);
		i = l % 3600;
		sprintf(cp,"%d%d", (i/60) / 10, (i/60) % 10);
		END(cp);
	} else {
		i = l;
		sprintf(cp,"%d", i / 60);
		END(cp);
	}
	i %= 60;
	*cp++ = ':';
	sprintf(cp,"%d%d", i / 10, i % 10);
}

/*
 *			N R E A D
 */
Nread(int fd, char *buf, int count )
{
	struct sockaddr_in from;
	int len = sizeof(from);
	register int cnt;
	if( udp )  {
		cnt = recvfrom( fd, buf, count, 0, &from, &len );
		numCalls++;
	} else {
		if( b_flag )
			cnt = mread( fd, buf, count );	/* fill buf */
		else {
			cnt = read( fd, buf, count );
			numCalls++;
		}
	}
	return(cnt);
}

/*
 *			N W R I T E
 */
Nwrite(int fd, char *buf, int count )
{
	register int cnt;
        long i; 
        if (rate>0) for(i=0;i<rate*100;i++); 
	if( udp )  {
again:
		cnt = sendto( fd, buf, count, 0, &sinhim, sizeof(sinhim) );
		numCalls++;
		if( cnt<0 && errno == ENOBUFS )  {
			delay(18000);
			errno = 0;
			goto again;
		}
	} else {
		cnt = write( fd, buf, count );
		numCalls++;
	}
	return(cnt);
}

delay(us)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = us;
	(void)select( 1, (fd_set *)0, (fd_set *)0, (fd_set *)0, &tv );
	return(1);
}

/*
 *			M R E A D
 *
 * This function performs the function of a read(II) but will
 * call read(II) multiple times in order to get the requested
 * number of characters.  This can be necessary because
 * network connections don't deliver data with the same
 * grouping as it is written with.  Written by Robert S. Miles, BRL.
 */
int
mread(fd, bufp, n)
int fd;
register char	*bufp;
unsigned	n;
{
	register unsigned	count = 0;
	register int		nread;

	do {
		nread = read(fd, bufp, n-count);
		numCalls++;
		if(nread < 0)  {
			perror("ttcp_mread");
			return(-1);
		}
		if(nread == 0)
			return((int)count);
		count += (unsigned)nread;
		bufp += nread;
	 } while(count < n);

	return((int)count);
}
