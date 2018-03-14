

#include <iostream>
#include <thread>

using namespace std;

#include "co_routine.h"

#include "socket_tool.h"
#include "thread_cls.h"
#include "link_cls.h"

static void * accept_routine_c(void*arg){
	thread_cls *p_thread = (thread_cls *)arg;
	p_thread->accept_routine();
	return 0;
}

static void * release_routine_c(void*arg){
	thread_cls *p_thread = (thread_cls *)arg;
	p_thread->release_routine();
	return 0;
}

void put2release(void *thread,int fd){
	thread_cls *p_thread = (thread_cls *)thread;
	p_thread->put2release(fd);
}

thread_cls::thread_cls(int id,int listen_fd){
	m_thread_id = id;
	m_listen = listen_fd;
	m_accept_co = NULL;
	m_release_co = NULL;
}

thread_cls::~thread_cls(){
cout << __FILE__<<" "<<__FUNCTION__<<endl;

	if(m_accept_co!=NULL){
		co_release(m_accept_co);
		m_accept_co = NULL;
	}
	if(m_release_co!=NULL){
		co_release(m_release_co);
		m_release_co = NULL;
	}
}

void thread_cls::run(void){

	cout << "thread_id:"<<m_thread_id<<endl;
	co_create( &m_accept_co,NULL,accept_routine_c,this);
	co_resume( m_accept_co );

	
	co_create( &m_release_co,NULL,release_routine_c,this);
	co_resume( m_release_co );

	co_eventloop( co_get_epoll_ct(),0,0 );
}

int co_accept(int fd, struct sockaddr *addr, socklen_t *len);
void thread_cls::accept_routine(void){
	co_enable_hook_sys();
	//printf("accept_routine\n");
	while(1){
		struct pollfd pf = { 0 };
		pf.fd = m_listen;
		pf.events = (POLLIN|POLLERR|POLLHUP);
		int iret = co_poll( co_get_epoll_ct(),&pf,1,-1);
		if(iret > 0){
			struct sockaddr_in addr; //maybe sockaddr_un;
			memset( &addr,0,sizeof(addr) );
			socklen_t len = sizeof(addr);
			int fd = co_accept(m_listen, (struct sockaddr *)&addr, &len);
			if(fd<=0){
				cerr << __FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<< " co_accept ret <= 0"<<endl;
				exit(-1);
			}
			cout << "thread_id:"<<m_thread_id<<" accept fd:"<<fd<<endl;

			SetNonBlock( fd );

			try{
				shared_ptr<link_cls> p = make_shared<link_cls>(this,fd);
				m_map_run[fd]=p;
			}catch (exception& e){
				cerr << __FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<<endl;
				std::cerr << "Exception: " << e.what() << "\n";
				exit(-1);
			}
		}else if(iret == 0){/////time out
			
		}else{
				cerr << __FILE__<<" "<<__FUNCTION__<<" "<<__LINE__<< " co_poll ret < 0"<<endl;
				exit(-1);
		}
	}
}

void thread_cls::release_routine( void ){
	co_enable_hook_sys();

	for(;;){
		if(m_map_release.size()>0){
			m_map_release.clear();
		}
		co_poll( co_get_epoll_ct(),NULL,0,1000);
//cout << "m_map_release.size():"<<m_map_release.size()<<endl;
	}
}

void thread_cls::put2release(int fd){
	m_map_release[fd] = m_map_run[fd];
	m_map_run.erase(fd);
}

