/* osWrap_linux.c - DK 3.0 functions to hide os dependent calls */

/* 
Copyright (c) 2013 Qualcomm Atheros, Inc.
All Rights Reserved. 
Qualcomm Atheros Confidential and Proprietary. 
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#include "wlantype.h"

#include "athreg.h"
#include "manlib.h"
#include "UserPrint.h"
#include "MyDelay.h"

#ifndef SOC_AP
#include "mInst.h"
#endif

#include "termios.h"
#include <fcntl.h>


#include "Socket.h"
#include "wlantype.h"

struct termios oldtio;

#define COM1 "/dev/ttyS0"
#define COM2 "/dev/ttyS1"

#define SEND_BUF_SIZE        1024

char terminationChar = '\n';

#define uiPrintf UserPrint


extern A_UINT32 sent_bytes;
static A_UINT32 os_com_open(struct _Socket *pOSSock);
static A_UINT32 os_com_close(struct _Socket *pOSSock);
extern HANDLE open_device(A_UINT32 device_fn, A_UINT32 devIndex, char*);
int socketListen (struct _Socket *pOSSock);

static A_INT32 socketConnect
(
 	char *target_hostname, 
	int target_port_num, 
	A_UINT32 *ip_addr
);


long SocketRead(struct _Socket *pSockInfo, char *buf, long len )
{
	int nread;
	int ncopy;
	int il;

	// printf("SocketRead start\n");
	ncopy=0;

	// Make the socket non blocking
	fcntl(pSockInfo->sockfd, F_SETFL, O_NONBLOCK);

	// try to get some more data
	//
	nread=MSOCKETBUFFER-pSockInfo->nbuffer;

    nread= (int)recv(pSockInfo->sockfd, &(pSockInfo->buffer[pSockInfo->nbuffer]), nread, 0);
	//if(nread>0){
		//printf("actually read %d, %s\n",nread, pSockInfo->buffer);		
	//}
	if(nread<0) {
	    if (errno == EINTR || errno == EAGAIN) {
	        nread = 0;
	    }
	    else {
            uiPrintf( "ERROR::socket receive: %s\n", strerror(errno));
            return -1;
        }
	}
	pSockInfo->nbuffer+=nread;
	//
	// check to see if we have enough stored data to return a message
	//
	for(il=0; il<pSockInfo->nbuffer; il++)
	{
		if ( pSockInfo->buffer[il] == terminationChar )
		{
			//printf("found eol at %d\n",il);
			il++;
			//
			// copy data to user buffer
			//
			if(len<il)
			{
				ncopy = (int)(len - 1);
			}
			else
			{
				ncopy=il;
			}
			memcpy(buf,pSockInfo->buffer,ncopy);
			buf[ncopy]=0;
			//
			// compress the internal data
			//
			memcpy(pSockInfo->buffer,&pSockInfo->buffer[il],pSockInfo->nbuffer-il);
			pSockInfo->nbuffer -= il;

			return ncopy;
		}
	}
	//
	// no message, but buffer is full. This is a problem.
	// delete allof the existing data.
	//
	if(il>=MSOCKETBUFFER)
	{
		printf("socketRead: buffer full, discarding input\n");
		pSockInfo->nbuffer=0;
	}
    return ncopy;

}

/**************************************************************************
* osSockWrite - write len bytes into the socket, pSockInfo, from *buf
*
* This routine calls a OS specific routine for socket writing
* 
* RETURNS: length read
*/
long SocketWrite(struct _Socket *pSockInfo, char *buf, long len)
{

		int	dwWritten;
		long bytes,cnt;
		unsigned char* bufpos; 
		long tmp_len;

	    tmp_len = len;
	   	bufpos = (unsigned char *)buf;
		dwWritten = 0;

	    while (len) 
		{
	        if (len < SEND_BUF_SIZE) bytes = len;
	        else bytes = SEND_BUF_SIZE;
	    	
	        cnt = send(pSockInfo->sockfd, (char *)bufpos, bytes, 0);
           	if (cnt == -1) {
           		if (errno == EINTR || errno == EAGAIN) {
           			continue;
           		} else {
           			uiPrintf("Warning:: Socket write: %s\n", strerror(errno));
           			return -1;
           		}
       		}
	   		dwWritten += cnt;
	    	len  -= cnt;
	    	bufpos += cnt;
		}
        len = tmp_len;
    
        if (dwWritten != len) {
	        dwWritten = 0;
        }
    	return dwWritten;
}


#if 0

A_INT32 SocketWrite
(
	struct _Socket           *pSockInfo,
	A_UINT8                *buf,
	A_INT32                len
	)
{
	A_UINT32		res, iIndex;

	q_uiPrintf("SocketWrite:%d\n", len);
   

	res = fd_write(pSockInfo->sockfd, pSockInfo->port_num, (A_UINT8 *) buf, len);

    q_uiPrintf("Sent %d bytes of %d bytes\n", res, len);
    for(iIndex=0; iIndex<len; iIndex++) {
            q_uiPrintf("%x ", buf[iIndex]);
            if (!(iIndex % 32)) q_uiPrintf("\n");
    }

	if (res != len) {
		return 0;
	}
	else {
		return len;
	}
}
#endif

#if 0
struct _Socket *osSockConnect
(
	char *mach_name
)
{
	struct _Socket *pOSSock;
	int res;
	char *cp;
    A_UINT32 err;
   HANDLE       hDevice;

        //uiPrintf("SNOOP:: osSockConnect called\n");
    pOSSock = (struct _Socket *) malloc(sizeof(*pOSSock));
    if (!pOSSock) {
       uiPrintf("ERROR::osSockConnect: malloc failed for pOSSock \n");
       return NULL;
    }


	while (*mach_name == '\\') {
		mach_name++;
	}
	
	for (cp = mach_name; (*cp != '\0') && (*cp != '\\'); cp++) {
	}
	*cp = '\0';

	q_uiPrintf("osSockConnect: starting mach_name = '%s'\n", mach_name);

	if (!strcmp(mach_name, ".")) {
			mach_name = "localhost";
     }

	  strcpy(pOSSock->hostname, mach_name);
	  pOSSock->port_num = -1;  // ???

    if (!strcasecmp(mach_name, "COM1")) {
	    q_uiPrintf("osSockConnect: Using serial communication port 1\n");
	    strcpy(pOSSock->hostname, COM1);
		pOSSock->port_num = COM1_PORT_NUM;
    }
    if (!strcasecmp(mach_name, "COM2")) {
	    q_uiPrintf("osSockConnect: Using serial communication port 2\n");
	    strcpy(pOSSock->hostname, COM2);
		pOSSock->port_num = COM2_PORT_NUM;
    }
	if (!strcasecmp(pOSSock->hostname, "SDIO")) {
       pOSSock->port_num = MBOX_PORT_NUM;
	}
	pOSSock->sockDisconnect = 0;
	pOSSock->sockClose = 0;

    switch(pOSSock->port_num) {
       case MBOX_PORT_NUM: 
            hDevice = open_device(SDIO_FUNCTION, 0, NULL);
		    pOSSock->sockfd = hDevice;
			break;
       case COM1_PORT_NUM:
       case COM2_PORT_NUM:
        if ((err=os_com_open(pOSSock)) != 0) {
            uiPrintf("ERROR::osSockConnect::Com port open failed with error = %x\n", err);
            exit(0);
        }
		break;
       case SOCK_PORT_NUM: {
			q_uiPrintf("osSockConnect: revised mach_name = '%s':%d\n", pOSSock->hostname, pOSSock->port_num);
	  		res = socketConnect(pOSSock->hostname, pOSSock->port_num, &pOSSock->ip_addr);
	  		if (res < 0) {
	          uiPrintf("ERROR::osSockConnect: pipe connect failed\n"); free(pOSSock);
	          return NULL;
	  		}
	  		q_uiPrintf("osSockConnect: Connected to pipe\n");

	  		q_uiPrintf("ip_addr = %d.%d.%d.%d\n",(pOSSock->ip_addr >> 24) & 0xff,
											(pOSSock->ip_addr >> 16) & 0xff,
											(pOSSock->ip_addr >> 8) & 0xff,
											(pOSSock->ip_addr >> 0) & 0xff);
	  		pOSSock->sockfd = res;

		} // end of else
	}

	return pOSSock;
}
#endif


/**************************************************************************
* SocketClose - close socket
*
* Close the handle to the socket
*
*/
void SocketClose
(
	struct _Socket *pOSSock
)
{

	A_UINT32 err;

	if ((err=close(pOSSock->sockfd))) {
    	uiPrintf("SocketClose::close failure with errror = %x\n", err);
	}

	free(pOSSock);
	return;
}


static A_INT32 socketConnect
(
 	char *target_hostname, 
	int target_port_num, 
	A_UINT32 *ip_addr
)
{
	A_INT32		sockfd;
	A_INT32		res;
	struct sockaddr_in	sin;
	struct hostent *hostent;
	A_INT32		i;
	A_INT32		j;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
		q_uiPrintf("ERROR::socket failed: %s\n", strerror(errno));
		return -1;
   	}

	/* Allow immediate reuse of port */
    q_uiPrintf("setsockopt SO_REUSEADDR start\n");
    i = 1;
    res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));
    if (res != 0) {
        uiPrintf("ERROR::socketConnect: setsockopt SO_REUSEADDR failed: %d\n",strerror(errno));
		close(sockfd);
	     return -1;
 	}
    q_uiPrintf("setsockopt SO_REUSEADDR end\n");


	/* Set TCP Nodelay */
    q_uiPrintf("setsockopt TCP_NODELAY start\n");
	i = 1;
    j = sizeof(i);
    res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
   	if (res == -1) {
		uiPrintf("ERROR::setsockopt failed: %s\n", strerror(errno));
		close(sockfd);
		return -1;
   	}	
	q_uiPrintf("setsockopt TCP_NODELAY end\n");


	q_uiPrintf("gethostbyname start\n");
    q_uiPrintf("socket_connect: target_hostname = '%s'\n", target_hostname);
    hostent = gethostbyname(target_hostname);
    q_uiPrintf("gethostbyname end\n");
    if (!hostent) {
        uiPrintf("ERROR::socketConnect: gethostbyname failed: %d\n",strerror(errno));
		close(sockfd);
	    return -1;
    }

	memcpy(ip_addr, hostent->h_addr_list[0], hostent->h_length);
	*ip_addr = ntohl(*ip_addr);
					
   	sin.sin_family = AF_INET;
	memcpy(&sin.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
	sin.sin_port = htons((short)target_port_num);


	for (i = 0; i < 20; i++) {
		q_uiPrintf("connect start %d\n", i);
	   	res = connect(sockfd, (struct sockaddr *) &sin, sizeof(sin));
		q_uiPrintf("connect end %d\n", i);
        if (res == 0) {
            break;
        }
        MyDelay(1000);
    }
	
    if (i == 20) {
        uiPrintf("ERROR::connect failed completely\n");
		close(sockfd);
        return -1;
    }
		
   	return sockfd;
}

#if 0
/* Returns number of bytes written, -1 if error */
static A_INT32 fd_write
	(
	A_INT32 fd,
    A_UINT16 port_num,
	A_UINT8 *buf,
	A_INT32 bytes
	)
{
    	A_INT32	cnt;
    	A_UINT8*	bufpos;

    	bufpos = buf;

    	while (bytes) {
			cnt = write(fd, bufpos, bytes);

			if (!cnt) {
				break;
			}
			if (cnt == -1) {
	    		if (errno == EINTR) {
				continue;
	    		}
	    		else {
				return -1;
	    		}
			}
		bytes -= cnt;
		bufpos += cnt;
    	}

    	return (bufpos - buf);
}
#endif

struct _Socket *SocketListen ( unsigned int port )
{
    struct _Socket *pOSSock; 
    
    pOSSock = (struct _Socket *) malloc(sizeof(*pOSSock));
    if (!pOSSock) {
            uiPrintf("ERROR::osSockListen: malloc failed for pOSSock \n");
            return NULL; 
    }

    pOSSock->port_num = port;
    pOSSock->sockDisconnect = 0;
    pOSSock->sockClose = 0;

    pOSSock->sockfd = socketListen(pOSSock);
    if (pOSSock->sockfd == -1) {
        uiPrintf("ERROR::Socket create failed \n");
        free(pOSSock);
        return NULL;
    }
    pOSSock->nbuffer=0;
    
    return pOSSock;
}


int socketListen (struct _Socket *pOSSock)
{
	A_INT32     sockfd;
	A_INT32     res;
	struct sockaddr_in  sin;
	A_INT32     i;
	A_INT32     j;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
          uiPrintf( "ERROR::socket failed: %s\n", strerror(errno));
          return -1;
	}

	// Allow immediate reuse of port
	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (A_INT8 *)&i, j);
	if (res == -1) {
		uiPrintf( "ERROR::setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
	if (res == -1) {
		uiPrintf( "ERROR::setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =  INADDR_ANY;
	sin.sin_port = htons(pOSSock->port_num);
	res = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)); 
	if (res == -1) { 
		uiPrintf( "ERROR::bind failed: %s\n", strerror(errno)); 
		return -1; 
	}
	res = listen(sockfd, 4);
	if (res == -1) { 
		uiPrintf( "ERROR::listen failed: %s\n", strerror(errno)); 
		return -1; 
	} 
						
	return sockfd;
}

/**************************************************************************
* SocketAccept - Wait for a connection
*
*/
struct _Socket *SocketAccept ( struct _Socket *pOSSock, int noblock )
{
	struct _Socket *pOSNewSock;
	A_INT32		i;
	A_INT32		sfd;
	struct sockaddr_in	sin;
	int         oldfl;

    //    uiPrintf("SNOOP:: SocketAccept called\n");
	
   	i = sizeof(sin);
   	
   	oldfl = fcntl(pOSSock->sockfd, F_GETFL); 
	if(noblock==1) {
	    if (!(oldfl & O_NONBLOCK))
	        fcntl(pOSSock->sockfd, F_SETFL, oldfl | O_NONBLOCK);
	}
	else {
	    if (oldfl & O_NONBLOCK)
            fcntl(pOSSock->sockfd, F_SETFL, oldfl & ~O_NONBLOCK);	    
	}
	
   	sfd = accept(pOSSock->sockfd, (struct sockaddr *) &sin, (socklen_t *)&i);


   	if (sfd == -1) {
		if(errno == EWOULDBLOCK) {

			return NULL;
		} else {

			uiPrintf( "ERROR::accept failed: %s\n", strerror(errno));
			return NULL;
		}
	}

	pOSNewSock = (struct _Socket *) malloc(sizeof(*pOSNewSock));
	if (!pOSNewSock) {
		uiPrintf("ERROR:: SocketAccept: malloc failed for pOSNewSock \n");
		return NULL;
	}
  
  	strcpy(pOSNewSock->hostname, inet_ntoa(sin.sin_addr));
	pOSNewSock->port_num = sin.sin_port;

	pOSNewSock->sockDisconnect = 0;
	pOSNewSock->sockClose = 0;
	pOSNewSock->sockfd = sfd;
    pOSNewSock->nbuffer = 0;

	return pOSNewSock;
}


static A_INT32 socket_create_and_accept
(
	A_INT32 port_num
)
{
	A_INT32     sockfd;
	A_INT32     res;
	struct sockaddr_in  sin;
	A_INT32     i;
	A_INT32     j;
	A_INT32     sfd;

	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sockfd == -1) {
          fprintf(stderr, "socket failed: %s\n", strerror(errno));
          return -1;
	}

	// Allow immediate reuse of port
	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (A_INT8 *)&i, j);
	if (res == -1) {
		fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	i = 1;
	j = sizeof(i);
	res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (A_INT8 *)&i, j);
	if (res == -1) {
		fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
		return -1;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr =  INADDR_ANY;
	sin.sin_port = htons(port_num);
	res = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin)); 
	if (res == -1) { 
		fprintf(stderr, "bind failed: %s\n", strerror(errno)); 
		return -1; 
	}
	res = listen(sockfd, 4);
	if (res == -1) { 
		fprintf(stderr, "listen failed: %s\n", strerror(errno)); 
		return -1; 
	} 
						
	i = sizeof(sin);
	sfd = accept(sockfd, (struct sockaddr *) &sin, (socklen_t *)&i);
	if (sfd == -1) { 
		fprintf(stderr, "accept failed: %s\n", strerror(errno)); 
		return -1; 
	} 
	
	res = close(sockfd); 
	if (res == -1) { 
		fprintf(stderr, "sockfd close failed: %s\n", strerror(errno)); 
		return 1; 
	}
								
	return sfd;
}

#ifndef WMI_PIPE
struct _Socket *osSockCreate
(
	char *pname
)
{
	struct _Socket *pOSSock;

    pOSSock = (struct _Socket *) malloc(sizeof(*pOSSock));
    if (!pOSSock) {
	        uiPrintf("osSockCreate: malloc failed for pOSSock \n");
	        return NULL;
    }

    strcpy(pOSSock->hostname, "localhost");
    pOSSock->port_num = -1;

    pOSSock->sockfd = socket_create_and_accept(pOSSock->port_num);

    if (pOSSock->sockfd == -1) {
	        uiPrintf("Socket create failed \n");
	        return NULL;
    }

    return pOSSock;
}

A_UINT32 os_com_open(struct _Socket *pOSSock)
{
    A_INT32 fd; 
    A_UINT32 nComErr=0;
    struct termios my_termios;

    fd = -1;
    fd = open(pOSSock->hostname, O_RDWR | O_NOCTTY  | O_SYNC);

    if (fd < 0) { 
        nComErr = -1; //nComErr | COM_ERROR_GETHANDLE;
        return nComErr;
    }
	

	tcgetattr(fd, &oldtio);
	// NOTE: you may want to save the port attributes
	// here so that you can restore them later
	q_uiPrintf("%s Terminal attributes\n", pOSSock->hostname);
    q_uiPrintf("old cflag=%08x\n", my_termios.c_cflag);
	q_uiPrintf("old oflag=%08x\n", my_termios.c_oflag);
	q_uiPrintf("old iflag=%08x\n", my_termios.c_iflag);
	q_uiPrintf("old lflag=%08x\n", my_termios.c_lflag);
	//q_uiPrintf("old line=%02x\n", my_termios.c_line);
	tcflush(fd, TCIFLUSH);
	memset(&my_termios,0,sizeof(my_termios));
	my_termios.c_cflag = B19200 | CS8 |CREAD | CLOCAL ;
	my_termios.c_iflag = IGNPAR;
	my_termios.c_oflag = 0;
	my_termios.c_lflag = 0; //FLUSHO; //|ICANON;
	my_termios.c_cc[VMIN] = 1;
	my_termios.c_cc[VTIME] = 0;
	tcsetattr(fd, TCSANOW, &my_termios);
	q_uiPrintf("new cflag=%08x\n", my_termios.c_cflag);
	q_uiPrintf("new oflag=%08x\n", my_termios.c_oflag);
	q_uiPrintf("new iflag=%08x\n", my_termios.c_iflag);
	q_uiPrintf("new lflag=%08x\n", my_termios.c_lflag);
	//q_uiPrintf("new line=%02x\n", my_termios.c_line);
	

    pOSSock->sockfd = fd;
    tcflush(pOSSock->sockfd, TCIOFLUSH);
    return 0;
}

A_UINT32 os_com_close(struct _Socket *pOSSock)
{   
    A_UINT32 nComErr;
    // reset error byte
    nComErr = 0;
    if (pOSSock->sockfd < 0) {
            nComErr = -1; //nComErr | COM_ERROR_INVALID_HANDLE;
            return nComErr;
    }

    tcsetattr(pOSSock->sockfd,TCSANOW,&oldtio);

    close(pOSSock->sockfd);
    pOSSock->sockfd = 0;

    return 0;
}
#endif //WMI_PIPE

PARSEDLLSPEC void SetStrTerminationChar( char tc )
{
	terminationChar = tc;
}

PARSEDLLSPEC char GetStrterminationChar( void )
{
	return terminationChar;
}

