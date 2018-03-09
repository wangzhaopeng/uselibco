
#include <iostream>
#include <memory>
#include <map>
#include <vector>
#include <thread>
using namespace std;

#include "co_routine.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <stack>

#include "link_cls.h"
#include "socket_tool.h"
#include "thread_cls.h"


int main(int argc,char *argv[])
{
	signal(SIGPIPE, SIG_IGN);////if remote close socket and local write 2times will send SIGPIPE cause exit,so ignore SIGPIPE

	const char *ip = "0";
	int port = 60000;

	int listen_fd = CreateTcpSocket( port,ip,true );
	listen( listen_fd,1024*1024 );
	if(listen_fd==-1){
		printf("Port %d is in use\n", port);
		return -1;
	}
	printf("listen %d %s:%d\n",listen_fd,ip,port);

	SetNonBlock( listen_fd );

	int core_cnt = 1;//thread::hardware_concurrency();
	cout << "core_cnt "<< core_cnt<<endl;

	vector<thread_cls*> pv_thread_cls(core_cnt);
	vector<thread*> pv_thread(core_cnt);
	for(int i = 0; i < core_cnt; i++){
		thread_cls* p_thread_cls = new thread_cls(i+1,listen_fd);
		thread* p_thread = new thread(&thread_cls::run,p_thread_cls);

		pv_thread_cls[i] = p_thread_cls;
		pv_thread[i] = p_thread;
	}

	for(int i = 0; i < core_cnt; i++){
		pv_thread[i]->join();
	}

	cout << "return "<<endl;
	return 0;
}

