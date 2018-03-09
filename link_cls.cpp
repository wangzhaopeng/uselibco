
#include "iostream"
using namespace std;

#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>


#include "co_routine.h"

#include "link_cls.h"
#include "socket_tool.h"

static void * run_c(void*arg);

link_cls::~link_cls(void)
{
	cout << __FILE__<<" "<<__FUNCTION__<<endl;
	if(m_fd>0){
		close(m_fd);
		m_fd = 0;
	}
	if(m_co!=NULL){
		co_release(m_co);
		m_co = NULL;
	}
	
}

link_cls::link_cls(void *p_parent_thread,int fd)
{
	mp_parent_thread = p_parent_thread;
	m_fd = fd;
	co_create( &m_co,NULL,run_c,this);
	co_resume( m_co );
}





void put2release(void *thread,int fd);
void link_cls::run(void)
{
	co_enable_hook_sys();

	int fd = m_fd;

	//cout << "time "<<time(NULL)<<endl;
	while(1){
		char buf[1024]={0};
		int ret = ReadSocket(fd,buf,sizeof(buf),-1);
		cout << "fd:"<<fd<<" rcv:"<<ret<<endl;
		if( ret > 0 )
		{

			ret = WriteSocket( fd,buf,ret );////echo 

			{/////create tcp client and connect to server
			int fd2 = CreateTcpCliSocket(60001 ,"0",1*1000);
			if(fd2> 0){
				WriteSocket( fd2,buf,ret );
				memset(buf,0,sizeof(buf));
				ret = ReadSocket(fd2,buf,sizeof(buf),1000*5);
				cout << "ret " << ret <<" "<< buf<<endl;
				if(ret>0){
					ret = WriteSocket( fd,buf,ret );////get data and echo 
				}
				close(fd2);
			}
			}
		}
		if( ret <= 0 )
		{
			// accept_routine->SetNonBlock(fd) cause EAGAIN, we should continue
			//if (errno == EAGAIN)
			//	continue;
			//close( fd );
			//put2release(fd);
			put2release(mp_parent_thread,fd);
			break;
		}
	}
//cout << "time "<<time(NULL)<<endl;
}

static void * run_c(void*arg)
{
	link_cls *p_link = (link_cls *)arg;
	p_link->run();

	return 0;
}

