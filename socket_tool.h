#ifndef __SOCKET_TOOL_H__
#define __SOCKET_TOOL_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

#include "co_routine.h"

static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

static void SetAddr(const char *pszIP,const unsigned short shPort,struct sockaddr_in &addr)
{
	bzero(&addr,sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(shPort);
	int nIP = 0;
	if( !pszIP || '\0' == *pszIP   
	    || 0 == strcmp(pszIP,"0") || 0 == strcmp(pszIP,"0.0.0.0") 
		|| 0 == strcmp(pszIP,"*") 
	  )
	{
		nIP = htonl(INADDR_ANY);
	}
	else
	{
		nIP = inet_addr(pszIP);
	}
	addr.sin_addr.s_addr = nIP;

}

static int CreateTcpSocket(const unsigned short shPort /* = 0 */,const char *pszIP /* = "*" */,bool bReuse /* = false */)
{
	int fd = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
	if( fd >= 0 )
	{
		if(shPort != 0)
		{
			if(bReuse)
			{
				int nReuseAddr = 1;
				setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&nReuseAddr,sizeof(nReuseAddr));
			}
			struct sockaddr_in addr ;
			SetAddr(pszIP,shPort,addr);
			int ret = bind(fd,(struct sockaddr*)&addr,sizeof(addr));
			if( ret != 0)
			{
				close(fd);
				return -1;
			}
		}
	}
	return fd;
}


static int CreateTcpCliSocket(const unsigned short shPort ,const char *pszIP,int outms)
{
	co_enable_hook_sys();
	int fd = socket(PF_INET, SOCK_STREAM, 0);
	SetNonBlock( fd );

	struct sockaddr_in addr;
	SetAddr(pszIP, shPort, addr);

	/////NonBlock will return -1  errno 115(EINPROGRESS)
	int ret = connect(fd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret == -1 && errno == EINPROGRESS){
		struct pollfd pf = { 0 };
		pf.fd = fd;
		pf.events = (POLLOUT|POLLERR|POLLHUP);
		co_poll( co_get_epoll_ct(),&pf,1,outms);

		int error = 0;
		uint32_t socklen = sizeof(error);

		/////error 111(ECONNREFUSED)
		ret = getsockopt(fd, SOL_SOCKET, SO_ERROR,(void *)&error,  &socklen);
		//cout <<"ret "<<ret<< " error "<<error<<endl;
		if(ret == 0 && error == 0){
			return fd;
		}
	}

	close(fd);
	return -1;
}

static int ReadSocket(int fd, void *buf, size_t nbyte, int outms)
{
	struct pollfd pf = { 0 };
	pf.fd = fd;
	pf.events = (POLLIN|POLLERR|POLLHUP);
	co_poll( co_get_epoll_ct(),&pf,1,outms);

	int ret = read( fd,buf,nbyte );
	return ret;
}

static int CoSleep(int ms)
{
	//if(ms == 0){
	//	return 0;
	//}else{
	//	int ret = co_poll( co_get_epoll_ct(),NULL,0,ms);//////use this if ms==0 find Segmentation fault
	//}
	int ret = poll( NULL,0,ms);
	return ret;
}

static int WriteSocket(int fd, const void *buf, size_t nbyte)
{
	////return -1 errno 32(EPIPE  Broken pipe,remote close)
	int ret = write( fd,buf,nbyte );
	//cout << "w fd:" << fd<<" ret:"<<ret<<" errno:"<<errno<<endl;
	return ret;
}

#endif
