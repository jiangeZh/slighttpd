/*************************************************************************
    > File Name: master.cpp
    > Author: Jiange
    > Mail: jiangezh@qq.com 
    > Created Time: 2016年01月27日 星期三 19时37分23秒
 ************************************************************************/

#include "master.h"
#include "worker.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>

Master::Master(const std::string &ip, unsigned short port)
	:m_worker(ip, port)
{
	m_base = NULL;
	m_exit_event  = NULL;
	m_chld_event  = NULL;
	nums_of_child = 4;
}

Master::~Master()
{
	if (m_base)
	{
        event_free(m_exit_event);
        event_free(m_chld_event);
		event_base_free(m_base);
	}
	std::cout << "Master Closed" << std::endl;
}


bool Master::StartMaster()
{
	std::cout << "Start Master" << std::endl;

	if (!m_worker.Init(this))
	{
		return false;
	}

	//创建一定数量的worker
	while (nums_of_child > 0)
	{
		switch (fork())
		{
			case -1:
				return false;
			case 0:
			{
				m_worker.Run();
				return true;
			}	
			default:
				--nums_of_child;
				break;
		}
	}

	m_base = event_base_new();
	m_exit_event = evsignal_new(m_base, SIGINT, Master::MasterExitSignal, m_base);
	m_chld_event = evsignal_new(m_base, SIGCHLD, Master::MasterChldSignal, this);
	evsignal_add(m_exit_event, NULL);
	evsignal_add(m_chld_event, NULL);
	event_base_dispatch(m_base);

	return true;
}

void Master::MasterExitSignal(evutil_socket_t signo, short event, void *arg)
{	
	//kill(0, SIGINT); //终端下是不需要的，因为所以子进程跟终端关联，所以都会收到INT
	event_base_loopexit((struct event_base*)arg, NULL);
}


void Master::MasterChldSignal(evutil_socket_t signo, short event, void *arg)
{	
	Master *master = (Master *)arg;
	pid_t	pid;
	int		stat;
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0)
	{
		++(master->nums_of_child);
		std::cout << "Child " << pid << " terminated" << std::endl;
	}
}
