
#ifndef __THREAD_CLS_H__
#define __THREAD_CLS_H__

#include <map>
#include <memory>

#include "co_routine.h"

#include "link_cls.h"

class thread_cls
{
public:
	thread_cls(int id,int listen_fd);
	~thread_cls(void);

	void run(void);
	void accept_routine(void);
	void release_routine(void);
	void put2release(int fd);

	int m_listen;
	int m_thread_id;
	std::map<int,shared_ptr<link_cls>> m_map_run;
	std::map<int,shared_ptr<link_cls>> m_map_release;

	stCoRoutine_t *m_accept_co;
	stCoRoutine_t *m_release_co;
	stCoRoutineAttr_t m_CoRoutineAttr;
};

#endif
