#ifndef __SOCKET_TOOL_H__
#define __SOCKET_TOOL_H__

#include <vector>
#include <deque>

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>

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

static int WritePack(int fd,const void *buf, size_t nbyte)
{
	if (nbyte <= 0 || nbyte >= 0xffff){
		return -1;
	}
	vector<unsigned char> v_buf(1);
	v_buf[0] = 0xf0;
	unsigned short ns; 
	ns = htons(nbyte);
	unsigned char*puc = (unsigned char*)&ns;
	v_buf.insert(v_buf.end(), puc,puc+2);
	puc = (unsigned char*)buf;
	v_buf.insert(v_buf.end(),puc,puc+nbyte);
	return WriteSocket(fd,&v_buf[0],v_buf.size());
}

//////发现一个完整包就返回包长度，否则返回0
static int check_dq(deque<unsigned char>&dq)
{
	unsigned char h0;
	unsigned short len;

	while (dq.size() >= 3){
		h0 = dq[0];
		if (h0 != 0xf0){
			dq.pop_front();
			continue;
		}
		unsigned char *puc = (unsigned char *)(&len);
		puc[0] = dq[1]; puc[1] = dq[2];
		len = ntohs(len);
		if (dq.size() >= len + 3){
			return len;
		}else{
			return 0;
		}
	}
	return 0;
}

static int get_pack(deque<unsigned char>&dq, vector<unsigned char>&vdata)
{
	vdata.clear();

	int len = check_dq(dq);
	if (len > 0){
		vdata.insert(vdata.end(), dq.begin()+3, dq.begin()+3+len);
		dq.erase(dq.begin(), dq.begin() + 3 + len);
	}
	
	return len;
}

static int ReadPack(int fd, deque<unsigned char>&dq, vector<unsigned char>&vdata, int outms)
{
	int ret;
	ret = get_pack(dq,vdata);//cout << "get_pack 1 ret:"<<ret<<endl;
	if(ret > 0){
		return ret;
	}

	struct timeval tv0;
	gettimeofday (&tv0,NULL);
	unsigned long long ms0 = tv0.tv_sec*1000+tv0.tv_usec/1000;
	
	int waitms = outms;

	vector<unsigned char> v_buf(1024);

	while(1){
		ret = ReadSocket(fd,&v_buf[0],v_buf.size(),waitms);//cout << "read ret:"<<ret<<endl;
		if(ret <= 0){///error timeout eof
			return ret;
		}else{
			dq.insert(dq.end(),v_buf.begin(),v_buf.begin()+ret);
			ret = get_pack(dq,vdata);//cout << "get_pack 2 ret:"<<ret<<endl;
			
			if(ret > 0){/////find ok
				return ret;
			}else{
				if(outms > 0){
					struct timeval tvn;
					gettimeofday (&tvn,NULL);
					unsigned long long msn = tvn.tv_sec*1000+tvn.tv_usec/1000;

					waitms = ms0+outms-msn;

					if(waitms<=0){
						return -1;
					}
				}
			}
		}
	}

	return ret;
}

#endif
