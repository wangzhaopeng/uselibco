
#ifndef __LINK_CLS_H__
#define __LINK_CLS_H__



class link_cls
{
public:
	link_cls(void *p_parent_thread,int fd);
	~link_cls(void);
	void run(void);

	stCoRoutine_t *m_co;
	int m_fd;
	void *mp_parent_thread;
};

#endif
